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

#define ATOM_BUCKETS 64

#define atomIsXDigit(c) isxdigit(c)
#define atomIsAlnum(c)  isalnum(c)
#define atomIsAlpha(c)  isalpha(c)
#define atomIsDigit(c)  isdigit(c)
#define atomIsSpace(c)  isspace(c)
#define atomIsPunct(c)							\
  (c == '('  || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || \
   c == '\'' || c == '"' || c == ',')

#define atomIsTextNull(t) (t.head == 0 && t.tail == 0)


/**
 * Atom node memory pool
 */
struct
{
  atom_node_t** memories;
  atom_node_t*  head;
  size_t        arrayCount;
} atomNodePool = { NULL, NULL, 0 };


const atom_text_t ATOM_TEXT_NULL = { 0, 0 };

/**
 * Allocate node memory, getting from pool or craete new
 * @function: atomAllocateNode
 */
static atom_node_t* atomAllocateNode(void)
{
  if (!atomNodePool.head) {
    const size_t  bucketSize = ATOM_BUCKETS;
    const size_t  arrayCount = atomNodePool.arrayCount;
    const size_t  memorySize = sizeof(atom_node_t) * bucketSize; 
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
    node->next        = NULL;
    node->name        = ATOM_TEXT_NULL; /* Does not set in atomCreate */
    node->data.asText = ATOM_TEXT_NULL; /* Does not set in atomCreate */ 
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


/* @function: atomRelease
 */
void atomRelease(void)
{
  if (atomNodePool.memories) {
    for (int i = 0; i < atomNodePool.arrayCount; i++) {
      //atom_node_t* node = atomNodePool.memories[i];
      free(atomNodePool.memories[i]);
    }
    free(atomNodePool.memories);

    /* Garbage collecting
     */
    atomNodePool.memories   = NULL;
    atomNodePool.head       = NULL;
    atomNodePool.arrayCount = 0;
  }
}


/* @function: atomGetFileSize
 */
size_t atomGetFileSize(FILE* file)
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
bool atomLexerInit(atom_lexer_t* lexer, int type, void* context)
{
  assert(lexer != NULL);
  
  if (!context) {
    /* Out of memory
     */
    return false;
  }

  /* Set context depend on lexer type
   */
  switch (type) {
  case ATOM_LEXER_STREAM:
    lexer->length        = atomGetFileSize(context);
    lexer->buffer.stream = context;
    break;

  case ATOM_LEXER_STRING:
    lexer->length        = strlen(context);
    lexer->buffer.string = context;
    break;
    
  default:
    return false;
  }
  
  /* Setting fields before return
   */
  lexer->type      = type;
  lexer->line      = 1;
  lexer->column    = 1;
  lexer->cursor    = 0;
  lexer->errcode   = ATOM_ERROR_NONE;
  lexer->errcursor = -1;
  return true;
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
static char atomLexerGet(atom_lexer_t* lexer, int cursor)
{
  assert(lexer != NULL);
  if (lexer->type == ATOM_LEXER_STREAM) {
    FILE*  stream = lexer->buffer.stream;
    fpos_t pos = cursor;
    fsetpos(stream, &pos);
    return fgetc(stream);
  }
  return lexer->buffer.string[cursor];
}


/**
 * Get the char at the cursor position
 */
static char atomLexerPeek(atom_lexer_t* lexer)
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
    char result = fgetc(stream);
    fseek(stream, -sizeof(char), SEEK_CUR); /* Go back to cursor */
    return result;
  }

  return lexer->buffer.string[lexer->cursor];
}


/**
 * Goto next position and peek the char
 */
static char atomLexerNext(atom_lexer_t* lexer)
{
  assert(lexer != NULL);
  lexer->cursor++;
  char c = atomLexerPeek(lexer);
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

  char c = atomLexerPeek(lexer);
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

  char c = atomLexerPeek(lexer);
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
atom_node_t* atomCreate(atom_type_t type, atom_text_t name)
{
  atom_node_t* node = atomAllocateNode();
  if (!node) {
    /* @error: Out of memory
     */
    return NULL;
  }
  node->type      = type;
  node->parent    = NULL;
  node->children  = NULL;
  node->lastChild = NULL;
  node->next      = NULL;
  node->prev      = NULL;
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
    atomFreeNode(node);
  }
}


/* @function: atomToLong
 */
bool atomToLong(const char* text, atom_data_t* value)
{
  assert(text != NULL && value != NULL);

  const char* ptr = text;
  char c    = *ptr;
  int  sign = 1;
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


/* @function: atomToReal
 */ 
bool atomToReal(const char* text, atom_data_t* value)
{
  assert(text != NULL && value != NULL);
  
  const char* ptr = text;
  char c    = *ptr;
  int  sign = 1;
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
  
  char  c   = atomLexerPeek(lexer);
  uint32_t head = lexer->cursor;
  uint32_t tail = head;
  atom_node_t* node = NULL;
  if (c == '"') {
    /* Get character until terminated
     */
    c = atomLexerNext(lexer);
    while (c && c != '"') {
      tail++;
      c = atomLexerNext(lexer);
    }

    /* A valid text must terminated with '"'
     */
    if (c != '"') {
      atomLexerError(lexer, ATOM_ERROR_UNTERMINATED);
      return NULL;
    }

    /* Do the last job to finish read a text
     */
    atomLexerNext(lexer);
    node = atomNewText(ATOM_TEXT_NULL, (atom_text_t){ head, tail });
  } else {
    char  text[1024];
    char* ptr = text;
    /* Read sequence characters still meet a separator
     */
    while (c && !atomIsSpace(c) && !atomIsPunct(c)) {
      *ptr++ = c; tail++;
      c = atomLexerNext(lexer);
    }
    *ptr++ = 0;

    /* Parsing value of the token
     */
    atom_data_t value;
    if (atomToLong(text, &value)) {
      node = atomNewLong(ATOM_TEXT_NULL, value.asLong);
    } else if (atomToReal(text, &value)) {
      node = atomNewReal(ATOM_TEXT_NULL, value.asReal);
    } else {
      node = atomCreate(ATOM_NAME, (atom_text_t){ head, tail });
    }
  }

  /* Read without error
   */
  return node;
}


/**
 *
 */
static bool atomListToSingle(atom_node_t* node)
{
  atom_node_t* children  = node->children;
  atom_node_t* lastChild = node->lastChild;
  if (children && children == lastChild) {
    node->type = children->type;
    node->data = children->data;
    atomDelete(children);
    node->children = node->lastChild = NULL;
    return true;
  }
  return false;
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

  char c = atomLexerPeek(lexer);
  char close;
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
	  root->type        = ATOM_LIST;
	  root->data.asRoot = true;
	}
      } else {
	if (!atomIsRoot(root)) {
	  atom_node_t* list = atomNewList(ATOM_TEXT_NULL);
	  list->data.asRoot = true;
	  atomAddChild(list, root);
	  root = list;
	}
	if (node->type == ATOM_NAME) {
	  atomLexerError(lexer, ATOM_ERROR_UNEXPECTED);
	  atomDelete(node);
	  atomDelete(root);
	  return NULL;
	}
	atomAddChild(root, node);
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
  atomListToSingle(root);
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

  char c = atomLexerPeek(lexer);
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
      if (!atomIsRoot(root)) {
	atom_node_t* list = atomNewList(ATOM_TEXT_NULL); /* Root list */
	list->data.asRoot = true;
	atomAddChild(list, root);
	root = list;
      }
      atomAddChild(root, node);
    }
  }
  atomListToSingle(root);
  return root;
}


/* @function: atomToText
 */
static bool atomToText(atom_node_t* node, char* text, size_t size)
{
  assert(node != NULL);
  assert(text != NULL && size > 0);
  
  if (atomIsList(node)) {
    char* tptr = text;
    char  value[1024];
    atom_node_t* child = node->children;
    /* Write name to text
     */
    //if (!atomIsRoot(node) || !node->name)
      *tptr++ = '(';
    if (!atomIsTextNull(node->name)) {
      /* Open list with '(' character
       * We not use '[' or '{', but it's still valid in using and hand-edit
       */
      atom_text_t name = node->name;
      //while (*name) {
	//*tptr++ = *name++;
      //}
      *tptr++ = ' ';
    }

    /* Write children values
     */
    while (child) {
      /* Get child value
       */
      atomToText(child, value, sizeof(value) - 1);
      child = child->next;

      /* Copy text
       */
      size_t length = strlen(value);
      memcpy(tptr, value, length);
      tptr += length;
      if (child) *tptr++ = ' '; /* Must have a separator */
    }
    /* Close list
     */ 
    //if (!atomIsRoot(node) || !node->name)
      *tptr++ = ')';
    *tptr++ = 0;
  } else {
    char value[1024];
    switch (node->type) {
    case ATOM_LONG:
      sprintf(value, "%lld", atomGetLong(node));
      break;
    case ATOM_REAL:
      sprintf(value, "%lf", atomGetReal(node));
      break;
    case ATOM_TEXT:
      //sprintf(value, "\"%s\"", atomGetText(node));
      break;
    default:
      break;
    }
    if (!atomIsTextNull(node->name)) {
      //snprintf(text, size, "(%s %s)", node->name, value);
    } else {
      strcpy(text, value);
    }
  }

  return true;
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
bool atomSaveText(atom_node_t* node, char* buffer, size_t size)
{
  assert(node != NULL);
  assert(buffer != NULL && size > 0);
  return atomToText(node, buffer, size);
}


/* @function: atomSetName
 */
bool atomSetName(atom_node_t* node, atom_text_t name)
{
  assert(node != NULL);
  node->name = name;
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

  node->data.asLong = value;
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

  node->data.asReal = value;
  return true;
}


/* @function: atomSetText
 */
bool atomSetText(atom_node_t* node, atom_text_t value)
{
  assert(node != NULL);
  if (!atomIsText(node)) {
    return false;
  }
  node->data.asText = value;
  return true;
}


/* @function: atomAddChild
 */
void atomAddChild(atom_node_t* node, atom_node_t* child)
{
  assert(node != NULL);
  child->parent = node;
  if (atomIsList(child)) {
    child->data.asRoot = false;
  }
  if (node->lastChild) {
    child->prev     = node->lastChild; 
    node->lastChild = node->lastChild->next = child;
  } else {
    node->lastChild = node->children        = child;
  }
}


/* @function: atomPrint
 */
void atomPrint(atom_lexer_t* lexer, atom_node_t* node)
{
  static const char* types[] = {
    "ATOM_NONE",
    "ATOM_LIST",
    "ATOM_LONG",
    "ATOM_REAL",
    "ATOM_TEXT",
    "ATOM_NAME",
  };
 
  static int stack = 0;
  if (node) {
    for (int i = 0; i < stack; i++) {
      fputc(' ', stdout);
    }
    fputs(types[node->type], stdout);
    fputs(" - ", stdout);
    for (int i = node->name.head; i < node->name.tail; i++) {
      fputc(atomLexerGet(lexer, i), stdout);
    }
    switch (node->type) {
    case ATOM_LIST:
      stack++;
      atom_node_t* children = node->children;
      if (children == NULL) {
	fputs(" - (null)\n", stdout);
      } else {
	fputc('\n', stdout);
	while (children) {
	  atomPrint(lexer, children);
	  children = children->next;
	}
      }
      stack--;
      break;
      
    case ATOM_LONG:
      printf(" - %lld\n", atomGetLong(node));
      break;
      
    case ATOM_REAL:
      printf(" - %lf\n", atomGetReal(node));
      break;
      
    case ATOM_TEXT:
      fputs(" - \"", stdout);
      for (int i = node->data.asText.head; i < node->data.asText.tail; i++) {
	fputc(atomLexerGet(lexer, i), stdout);
      }
      fputc('\n', stdout);
      break;

    default:
      fputc('\n', stdout);
      break;
    }
  }
}
