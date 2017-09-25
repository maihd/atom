/**
 * Atom - file data format with s-expression
 *
 * @author: MaiHD
 * @license: Free to use
 * @copyright: MaiHD @ ${HOME}, 2017
 */


#include "atom.h"

#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <setjmp.h>

#define __STR__(x) __VAL__(x)
#define __VAL__(x) #x

#ifdef NDEBUG
#define atomLexerError(l, e) _atomLexerError(l, e)
#else
#define atomLexerError(l, e)						\
  printf("Error at:" __FILE__ ":" __STR__(__LINE__) ":%s\n", __func__);	\
  _atomLexerError(l, e)
#endif

#define atomStrLen  strlen
#define atomStrCpy  strcpy
#define atomStrCmp  strcmp

#define atomIsXDigit(c) isxdigit(c)
#define atomIsAlnum(c)  isalnum(c)
#define atomIsAlpha(c)  isalpha(c)
#define atomIsDigit(c)  isdigit(c)
#define atomIsSpace(c)  isspace(c)
#define atomIsPunct(c)							\
  (c == '('  || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || \
   c == '\'' || c == '"' || c == ',') 

/**
 * Error code
 */
enum {
  ATOM_ERROR_NONE,
  ATOM_ERROR_UNBALANCED,
  ATOM_ERROR_UNEXPECTED,
  ATOM_ERROR_UNTERMINATED,
};

/**
 * Atom lexer for parsing
 */
typedef struct
{
  enum {
    ATOM_LEXER_STRING,
    ATOM_LEXER_STREAM,
  } type;
  int    line;
  int    column;
  int    cursor;        /* Cursor position  */
  size_t length;        /* Length of buffer */
  union
  {
    atom_text_t string;
    FILE*       stream;
  } buffer;

  int    errcode;       /* Error code        */
  int    errcursor;     /* Position at error */
  
  /* No padding needed
   */
} atom_lexer_t;


/**
 * Atom node memory pool
 */
struct
{
  atom_node_t** memories;
  atom_node_t*  head;
  size_t        arrayCount;
} atomNodePool = { NULL, NULL, 0 };


/**
 * Allocate node memory, getting from pool or craete new
 * @function: atomAllocateNode
 */
static atom_node_t* atomAllocateNode(void)
{
  if (!atomNodePool.head) {
    const size_t  bucketSize = 64;
    const size_t  memorySize = sizeof(atom_node_t) * bucketSize; 
    const size_t  arrayCount = atomNodePool.arrayCount;
    atom_node_t** memories   = atomNodePool.memories;

    memories = realloc(memories, sizeof(atom_node_t*) * (arrayCount + 1));
    if (!memories) {
      /* @error: out of memory
       */
      return NULL;
    }
    memories[arrayCount] = malloc(memorySize);
    if (!memories[arrayCount]) {
      /* @error: out of memory
       */
      return NULL;
    }
    
    atom_node_t* node = memories[arrayCount];
    atomNodePool.head = node;
    for (int i = 0; i < bucketSize - 1; i++) {
      node->next = node + i;
    }
    node->next = NULL;
    atomNodePool.memories   = memories;
    atomNodePool.arrayCount = arrayCount + 1; 
  }

  atom_node_t* node = atomNodePool.head;
  atomNodePool.head = node->next;
  return node;
}


/**
 * Free node
 */
static void atomFreeNode(atom_node_t* node)
{
  if (atomNodePool.memories && node) {
    node->next = atomNodePool.head;
    atomNodePool.head = node;
  }
}


/** 
 * Get file size
 */
static size_t atomGetFileSize(FILE* file)
{
  assert(file != NULL);
  fpos_t prev = ftell(file);
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fsetpos(file, &prev);
  return size;
}

/** 
 * Create a lexer with streaming buffer
 */
static atom_lexer_t* atomCreateStreamLexer(FILE* stream)
{
  assert(stream != NULL);
  
  atom_lexer_t* lexer = malloc(sizeof(atom_lexer_t));
  if (!lexer) {
    /* Out of memory
     */
    return NULL;
  }

  /* No buffer generation before FILE is buffered stream
   * But if we use Unix descriptor, there is no need a buffer too
   * This is a pro compare to using string buffer
   */
  
  /* Setting fields before return
   */
  lexer->type      = ATOM_LEXER_STREAM;
  lexer->line      = 1;
  lexer->column    = 1;
  lexer->cursor    = 0;
  lexer->length    = atomGetFileSize(stream);
  lexer->buffer.stream = stream;
  lexer->errcode   = ATOM_ERROR_NONE;
  lexer->errcursor = -1;
  return lexer;
}


/** 
 * Create a lexer with string buffer
 */
static atom_lexer_t* atomCreateStringLexer(atom_ctext_t string)
{
  assert(string != NULL);
  
  atom_lexer_t* lexer = malloc(sizeof(atom_lexer_t));
  if (!lexer) {
    /* Out of memory
     */
    return NULL;
  }

  /* Generate buffer from string
   */
  size_t      length = atomStrLen(string);
  atom_text_t buffer = malloc(length * sizeof(atom_char_t));
  if (!buffer) {
    /* Out of memory
     */
    free(lexer);
    return NULL;
  }
  memcpy(buffer, string, length);    /* Faster than strcpy */

  /* Setting fields before return
   */
  lexer->type      = ATOM_LEXER_STRING;
  lexer->line      = 1;
  lexer->column    = 1;
  lexer->cursor    = 0;
  lexer->length    = length;
  lexer->buffer.string = buffer;
  lexer->errcode   = ATOM_ERROR_NONE;
  lexer->errcursor = -1;
  return lexer;
}


/**
 * Delete lexer, progress depend on the type of lexer
 */
static void atomDeleteLexer(atom_lexer_t* lexer)
{
  if (lexer) {
    if (lexer->type == ATOM_LEXER_STRING) {
      free(lexer->buffer.string);
    }
    free(lexer);
  }
}


/**
 * Check if lexer reach the end
 * We don't use inline modifier 
 * 'cause may has more process to do with
 */
static bool atomLexerIsEnd(atom_lexer_t* lexer)
{
  assert(lexer != NULL);
  return lexer->cursor >= lexer->length;
}

/**
 * Get the char at the cursor position
 */
static atom_char_t atomLexerPeek(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  if (atomLexerIsEnd(lexer)) {
    return 0;
  }

  if (lexer->type == ATOM_LEXER_STREAM) {
    FILE*  stream = lexer->buffer.stream;
    fpos_t cursor = ftell(stream);
    if (cursor != lexer->cursor) {
      cursor = lexer->cursor;
      fsetpos(stream, &cursor);
    }
    atom_char_t result;
    if (fread(&result, sizeof(atom_char_t), 1, stream) != 1) {
      /* Should never reach here
       */
      assert(false && "Read char from stream failed!");
      return -1;
    }
    fseek(stream, -sizeof(atom_char_t), SEEK_CUR); /* Go back to cursor */
    return result;
  }

  return lexer->buffer.string[lexer->cursor];
}


/**
 * Goto next position and peek the char
 */
static atom_char_t atomLexerNext(atom_lexer_t* lexer)
{
  assert(lexer != NULL);
  lexer->cursor++;
  atom_char_t c = atomLexerPeek(lexer);
  if (c == '\n') {
    lexer->line++;
    lexer->column = 1;
  } else {
    lexer->column++;
  }
  return c;
}


/**
 * Peek the char and goto next position
 */
/*static atom_char_t atomLexerRead(atom_lexer_t* lexer)
{
  assert(lexer != NULL);
  atom_char_t result = atomLexerPeek(lexer);
  atomLexerNext(lexer);
  return result;
  }*/


/**
 * Skip space
 */
static void atomSkipSpace(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  atom_char_t c = atomLexerPeek(lexer);
  while (atomIsSpace(c)) {
    c = atomLexerNext(lexer);
  }
}


/**
 * Skip comment
 */
static void atomSkipComment(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  atom_char_t c = atomLexerPeek(lexer);
  while (c && c != '\n' && c != '\r') {
    c = atomLexerNext(lexer);
  }
  atomLexerNext(lexer);
}


/**
 * Set lexer checkpoint for error check and jump back to caller
 */
/*
static bool atomLexerCheck(atom_lexer_t* lexer)
{
  assert(lexer != NULL);
  int code = setjmp(lexer->errbuf);
  return code > ATOM_ERROR_NONE;
}
*/

/**
 * Raise an error, jump back to caller with error code 
 */
static void _atomLexerError(atom_lexer_t* lexer, int errcode)
{
  assert(lexer != NULL);
  
  int line   = lexer->line;
  int column = lexer->column;
  int cursor = lexer->errcursor = lexer->cursor;
  int c      = atomLexerPeek(lexer);
  switch ((lexer->errcode = errcode)) {
  case ATOM_ERROR_UNBALANCED:
    fprintf(stderr, "[%d:%d:%d]: Unbalance '%c'\n", line, column, cursor, c);
    break;
    
  case ATOM_ERROR_UNEXPECTED:
    fprintf(stderr, "[%d:%d:%d]: Unexpected '%c'\n", line, column, cursor, c);
    break;

  case ATOM_ERROR_UNTERMINATED:
    fprintf(stderr, "[%d:%d:%d]: Unterminated '%c'\n", line, column, cursor, c);
    break;
    
  case ATOM_ERROR_NONE:
  default:
    break;
  }

  /* Show error line
   */
  /*if (lexer->type == ATOM_LEXER_STRING) {
    atom_text_t lineText = lexer->buffer.string + (cursor - column + 1);
    fputs(lineText, stderr);
  } else {
    lexer->cursor = cursor - column + 1;
    lexer->column = 1;
    for (int i = 0; i < column; i++) {
      atom_char_t tchar = atomLexerPeek(lexer);
      fputc(tchar, stderr);
    }
    fputc('\n', stderr);
  }*/
}


/* @function: atomCreate
 */
atom_node_t* atomCreate(atom_type_t type, atom_ctext_t name)
{
  atom_node_t* node = atomAllocateNode();
  if (!node) {
    /* @error: Out of memory
     */
    return NULL;
  }
  node->name         = NULL;
  node->type         = type;
  node->parent       = NULL;
  node->children     = NULL;
  node->lastChild    = NULL;
  node->next         = NULL;
  node->prev         = NULL;
  node->value.asText = NULL;
  atomSetName(node, name);
  return node;
}


/* @function: atomDelete
 */
void         atomDelete(atom_node_t* node)
{
  if (node) {
    /* Remove from parent
     */
    if (node->prev) {
      node->prev = node->next;
    }
    if (node->next) {
      node->next = node->prev;
    }
    if (node->parent) {
      if (node->parent->children == node) {
	node->parent->children = node->next;
      }
      if (node->parent->lastChild == node) {
	node->parent->lastChild = node->prev;
      }
    }
    
    /* Remove children
     */
    if (atomIsList(node)) {
      while (node->children) {
	atom_node_t* next = node->children->next;
	atomDelete(node->children);
	node->children = next;
      }
    }

    /* Free usage heap memory
     */ 
    /*if (node->name) {
      free(node->name);
    }
    if (atomIsText(node)) {
      free(node->value.asText);
      }*/
    atomFreeNode(node);
  }
}


static bool atomToLong(atom_ctext_t text, atom_value_t* value)
{
  assert(text != NULL && value != NULL);

  atom_ctext_t ptr  = text;
  atom_char_t  c    = *ptr;
  int          sign = 1;
  switch (c) {
  case '-':
    sign = -1;
    ptr++;
    break;
  case '+':
    ptr++;
    break;

  default:
    break;
  }

  value->asLong   = 0;
  while ((c = *ptr++)) {
    if (!atomIsDigit(c)) {
      return false;
    }
    value->asLong = value->asLong * 10 + (c - '0');
  }
  value->asLong  *= sign;
  return true;
}


static bool atomToReal(atom_ctext_t text, atom_value_t* value)
{
  assert(text != NULL && value != NULL);
  atom_ctext_t ptr  = text;
  atom_char_t  c    = *ptr;
  int          sign = 1;
  switch (c) {
  case '-':
    sign = -1;
    ptr++;
    break;
  case '+':
    ptr++;
    break;

  default:
    break;
  }

  bool        dotFound  = false;
  atom_real_t precision = 10;
  value->asReal         = 0.0;
  while ((c = *ptr++)) {
    if (c == '.') {
      if (dotFound) {
	return false;
      }
      dotFound = true;
      continue;
    } else if (!atomIsDigit(c)) {
      return false;
    }
    if (dotFound) {
      value->asReal = value->asReal + (c - '0') / precision;
      precision    *= 10;
    } else {
      value->asReal = value->asReal * 10 + (c - '0');
    }
  }
  value->asReal *= sign;
  return true;
}

static atom_node_t* atomRead(atom_lexer_t* lexer);
/**
 * Read an atom token
 */
static atom_node_t* atomReadAtom(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  /* Nothing to parse when its end
   */
  atomSkipSpace(lexer);
  if (atomLexerIsEnd(lexer)) {
    return NULL;
  }
  
  atom_char_t  text[1024];
  atom_char_t  c    = atomLexerPeek(lexer);
  atom_node_t* node = NULL;
  atom_text_t  ptr  = text;
  if (c == '"') {
    /* Get character until terminated
     */
    c = atomLexerNext(lexer);
    while (c && c != '"') {
      *ptr++ = c;
      c = atomLexerNext(lexer);
    }
    *ptr++ = 0;

    /* A valid text must terminated with '"'
     */
    if (c != '"') {
      atomLexerError(lexer, ATOM_ERROR_UNTERMINATED);
      return NULL;
    }

    /* Do the last job to finish read a text
     */
    atomLexerNext(lexer);
    node = atomNewText(NULL, text);
  } else {
    /* Read sequence characters still meet a separator
     */
    while (c && !atomIsSpace(c) && !atomIsPunct(c)) {
      *ptr++ = c;
      c = atomLexerNext(lexer);
    }
    *ptr++ = 0;

    /* Parsing value of the token
     */
    atom_value_t value;
    if (atomToLong(text, &value)) {
      node = atomNewLong(NULL, value.asLong);
    } else if (atomToReal(text, &value)) {
      node = atomNewReal(NULL, value.asReal);
    } else {
      node = atomCreate(ATOM_NAME, text);
    }
  }

  /* Read without error
   */
  return node;
}


/**
 * Read list
 */
static atom_node_t* atomReadList(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  if (atomLexerIsEnd(lexer)) {
    return NULL;
  }

  atom_char_t c = atomLexerPeek(lexer);
  atom_char_t close;
  switch (c) {
  case '(': close = ')'; break;
  case '[': close = ']'; break;
  case '{': close = '}'; break;
  default:
    atomLexerError(lexer, ATOM_ERROR_UNEXPECTED);
    break;
  }

  c = atomLexerNext(lexer);
  atom_node_t* root = NULL;
  atom_node_t* node = NULL;
  do {
    /* Read child token, only parse when token is valid
     */
    atomSkipSpace(lexer);
    if ((node = atomRead(lexer))) {
      if (!root) {
	root = node;
	if (root->type == ATOM_NAME) {
	  root->type         = ATOM_LIST;
	  root->value.asRoot = true;
	}
      } else {
	if (!atomIsList(root) || !root->value.asRoot) {
	  atom_node_t* list = atomNewList(NULL);
	  list->value.asRoot = true;
	  list->children     = root;
	  list->lastChild    = root;
	  root = list;
	}
	if (node->type == ATOM_NAME) {
	  atomLexerError(lexer, ATOM_ERROR_UNEXPECTED);
	  atomDelete(node);
	  atomDelete(root);
	  return NULL;
	}
	if (root->lastChild) {
	  root->lastChild = root->lastChild->next = node;
	} else {
	  root->lastChild = root->children        = node;
	}
      }
    }

    /* Get next character to check if there is an end sign
     */
    atomSkipSpace(lexer);
    c = atomLexerPeek(lexer);
  } while (c && c != close && node);

  /* Must be end with ${close} character
   */
  if (c != close) {
    atomDelete(root);
    atomLexerError(lexer, ATOM_ERROR_UNBALANCED);
    return NULL;
  }
  atomLexerNext(lexer);

  /* 
   */
  if (root->children && root->children == root->lastChild) {
    root->type  = root->children->type;
    root->value = root->children->value;
    /* Delete children node 
     */
    atom_node_t* children  = root->children;
    children->name         = NULL;
    children->value.asText = NULL;
    atomDelete(children);
    /* Collect garbage
     */
    root->children = root->lastChild = NULL;
  }
  
  return root;
}


/**
 * Read a single token
 */
atom_node_t* atomRead(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  /* Leave 
   */
  atomSkipSpace(lexer);
  if (atomLexerIsEnd(lexer)) {
    return NULL;
  }

  atom_char_t c = atomLexerPeek(lexer);
  switch (c) {
  case ';':
    atomSkipComment(lexer);
    return atomRead(lexer);
      
  case '(':
  case '[':
  case '{':
    return atomReadList(lexer);
      
  case ')':
  case ']':
  case '}':
    atomLexerError(lexer, ATOM_ERROR_UNEXPECTED);
    return NULL;
    break;

  default:
    return atomReadAtom(lexer);
  }
  
  return NULL;
}


/**
 * Parse lexer data to atom
 */
atom_node_t* atomParse(atom_lexer_t* lexer)
{
  assert(lexer != NULL);

  //if (atomLexerCheck(lexer)) {
  //  return NULL;
  //}  
  
  atom_node_t* root = NULL;
  atom_node_t* node = NULL;
  atomSkipSpace(lexer);
  while (!atomLexerIsEnd(lexer)) {
    node = atomRead(lexer);

    /* NULL pointer handling
     */
    if (!node) {
      atomSkipSpace(lexer);
      if (lexer->errcode != ATOM_ERROR_NONE) {
	atomDelete(node);
	return NULL;
      }
      if (atomLexerIsEnd(lexer)) {
        break;
      }
      continue;
    }
    
    /* Append to root list or make root if root is not setted
     */
    if (!root) {
      root = node;
    } else {
      if (!atomIsList(root) || !root->value.asRoot) {
	atom_node_t* list   = atomNewList(NULL); /* Root list */
	list->value.asRoot  = true;
	list->children      = root;
	list->lastChild     = root;
	root = root->parent = list;
      }
      if (atomIsList(node)) {
	node->value.asRoot = false;
      }
      node->parent    = root;
      root->lastChild = root->lastChild->next = node;
    }
  }
  
  return root;
}


/* @function atomReadList
 */
//atom_node_t* atomReadList(atom_lexer_t* lexer)


/* @function: atomLoadFile
 */
atom_node_t* atomLoadFile(FILE* file)
{
  if (!file) {
    return NULL;
  }

  atom_lexer_t* lexer = atomCreateStreamLexer(file);
  if (!lexer) {
    /* Out of memory
     */ 
    return NULL;
  }
  atom_node_t* node = atomParse(lexer);

  /* Parsing finish
   */
  atomDeleteLexer(lexer);
  return node;
}


/* @function: atomLoadText
 */
atom_node_t* atomLoadText(atom_ctext_t buffer)
{
  if (!buffer) {
    /* Nothing to load/parse
     */
    return NULL;
  }
  
  atom_lexer_t* lexer = atomCreateStringLexer(buffer);
  if (!lexer) {
    /* Out of memory
     */ 
    return NULL;
  }
  atom_node_t* node = atomParse(lexer);

  /* Parsing finish
   */
  atomDeleteLexer(lexer);
  return node;
}


/* @function: atomSaveFile
 */
bool atomSaveFile(atom_node_t* node, FILE* file)
{
  assert(node != NULL);
  assert(file != NULL);
  return false;
}


/* @function: atomSaveText
 */
bool atomSaveText(atom_node_t* node, atom_text_t buffer, size_t size)
{
  assert(node != NULL);
  assert(buffer != NULL && size > 0);
  return false;
}

//atom_node_t* atomCreate(atom_ctext_t name, atom_value_t value);
//void         atomDelete(atom_node_t* node);

/* @function: atomSetName
 */
bool atomSetName(atom_node_t* node, atom_ctext_t name)
{
  assert(node != NULL);

  /* In special case, given name is NULL
   * We just simply setting name an empty string
   */
  if (!name) {
    if (node->name) {
      node->name[0] = 0;
    }
    return true;
  }
  
  size_t length = atomStrLen(name) + 1;
  char*  string = node->name;
  if (string) {
    size_t* header = (size_t*)((char*)string - sizeof(size_t));
    if (*header < length) {
      string = realloc(string, length * sizeof(atom_char_t));
    }
  } else {
    string = malloc(length * sizeof(atom_char_t));
  }
  memcpy(string, name, length * sizeof(atom_char_t));
  node->name = string;
  return true;
}


/* @function: atomSetLong
 */
bool atomSetLong(atom_node_t* node, atom_long_t value)
{
  assert(node != NULL);
  if (!atomIsLong(node)) {
    return false;
  }

  node->value.asLong = value;
  return true;
}


/* @function: atomSetReal
 */
bool atomSetReal(atom_node_t* node, atom_real_t value)
{
  assert(node != NULL);
  if (!atomIsReal(node)) {
    return false;
  }

  node->value.asReal = value;
  return true;
}


/* @function: atomSetText
 */
bool atomSetText(atom_node_t* node, atom_ctext_t value)
{
  assert(node != NULL);
  if (!atomIsText(node)) {
    return false;
  }
  
  /* In special case, given value is NULL
   * We just simply setting value an empty string
   */
  if (!value) {
    if (node->value.asText) {
      node->value.asText[0] = 0;
    }
    return true;
  }

  size_t      length = atomStrLen(value) + 1;
  atom_text_t string = node->value.asText;
  if (string) {
    size_t* header = (size_t*)((char*)string - sizeof(size_t));
    if (*header / sizeof(atom_char_t) < length) {
      string = realloc(string, length * sizeof(atom_char_t));
    }
  } else {
    string = malloc(length * sizeof(atom_char_t));
  }
  memcpy(string, value, length * sizeof(atom_char_t));
  node->value.asText = string;
  return true;
}
