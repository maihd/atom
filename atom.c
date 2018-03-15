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
#define atom_lexer_error(l, e) _atomLexerError(l, e)
#else
#define atom_lexer_error(l, e)						\
  printf("Error at:" __FILE__ ":" __STR__(__LINE__) ":%s\n", __func__);	\
  _atom_lexer_error(l, e)
#endif

#define ATOM_BUCKETS 64

#define atom_isxdigit(c) isxdigit(c)
#define atom_isalnum(c)  isalnum(c)
#define atom_isalpha(c)  isalpha(c)
#define atom_isdigit(c)  isdigit(c)
#define atom_isspace(c)  isspace(c)
#define atom_ispunct(c)							\
  (c == '('  || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || \
   c == '\'' || c == '"' || c == ',')


/**
 * Atom node memory pool
 */
struct
{
    atom_node_t** memories;
    atom_node_t*  head;
    size_t        count;
} atom_nodepool = { NULL, NULL, 0 };

/**
 * Allocate node memory, getting from pool or craete new
 * @function: atom_newnode
 */
static atom_node_t* atom_newnode(void)
{
    if (!atom_nodepool.head)
    {
	const size_t  bucketSize = ATOM_BUCKETS;
	const size_t  arrayCount = atom_nodepool.count;
	const size_t  memorySize = sizeof(atom_node_t) * bucketSize; 
	atom_node_t** memories   = atom_nodepool.memories;

	memories = realloc(memories,
			   sizeof(atom_node_t*) * (arrayCount + 1));
	if (!memories)
	{
	    /* @error: out of memory
	     */
	    return NULL;
	}
	
	memories[arrayCount] = malloc(memorySize);
	if (!memories[arrayCount])
	{
	    /* @error: out of memory
	     */
	    return NULL;
	}
    
	atom_node_t* node  = memories[arrayCount];
	atom_nodepool.head = node;
	for (int i = 0; i < bucketSize - 1; i++)
	{
	    node->next = node + i;
	}
	node->next         = NULL;
	node->name         = ATOM_TEXT_NULL;
	node->data.as_text = ATOM_TEXT_NULL;
	
	atom_nodepool.memories   = memories;
	atom_nodepool.count      = arrayCount + 1; 
    }

    atom_node_t*  node = atom_nodepool.head;
    atom_nodepool.head = node->next;
    return node;
}


/**
 * Free node
 */
static void atom_freenode(atom_node_t* node)
{
    if (atom_nodepool.memories && node)
    {
	node->next = atom_nodepool.head;
	atom_nodepool.head = node;
    }
}


/* @function: atomRelease
 */
void atom_release(void)
{
    if (atom_nodepool.memories)
    {
	for (int i = 0; i < atom_nodepool.count; i++)
	{
	    //atom_node_t* node = atomNodePool.memories[i];
	    free(atom_nodepool.memories[i]);
	}
	free(atom_nodepool.memories);

	/* Garbage collecting
	 */
	atom_nodepool.memories   = NULL;
	atom_nodepool.head       = NULL;
	atom_nodepool.count      = 0;
    }
}


/* @function: atomGetFileSize
 */
size_t atom_getfilesize(FILE* file)
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
int atom_lexer_init(atom_lexer_t* lexer, int type, void* context)
{
    assert(lexer != NULL);
  
    if (!context)
    {
	return ATOM_ERROR_ARGUMENTS;
    }

    /* Set context depend on lexer type
     */
    switch (type)
    {
    case ATOM_LEXER_STREAM:
	lexer->length        = atom_getfilesize(context);
	lexer->buffer.stream = context;
	break;

    case ATOM_LEXER_STRING:
	lexer->length        = strlen(context);
	lexer->buffer.string = context;
	break;
    
    default:
	return ATOM_ERROR_LEXERTYPE;
    }
  
    /* Setting fields before return
     */
    lexer->type      = type;
    lexer->line      = 1;
    lexer->column    = 1;
    lexer->cursor    = 0;
    lexer->errcode   = ATOM_ERROR_NONE;
    lexer->errcursor = -1;
    return ATOM_ERROR_NONE;
}


/**
 * Check if lexer reach the end
 * We don't use inline modifier 
 * 'cause may has more process to do with
 */
static atom_bool_t atom_lexer_iseof(atom_lexer_t* lexer)
{
    assert(lexer != NULL);
    return lexer->cursor >= lexer->length;
}



/**
 * Get the char at the cursor position
 */
static char atom_lexer_get(atom_lexer_t* lexer, int cursor)
{
    assert(lexer != NULL);
    switch (lexer->type)
    {
    case ATOM_LEXER_STREAM:
    {
	FILE*  stream = lexer->buffer.stream;
	fpos_t pos = cursor;
	fsetpos(stream, &pos);
	return fgetc(stream);
    }
    
    case ATOM_LEXER_STRING:
    {
	return lexer->buffer.string[cursor];
    }
    
    default:
	assert(0 && "Type of lexer (lexer->type) is invalid");
    }
}


/**
 * Get the char at the cursor position
 */
static char atom_lexer_peek(atom_lexer_t* lexer)
{
    assert(lexer != NULL);

    if (atom_lexer_iseof(lexer))
    {
	return 0;
    }

    switch (lexer->type)
    {
    case ATOM_LEXER_STREAM:
    {
	FILE*  stream = lexer->buffer.stream;
	fpos_t cursor = ftell(stream);
	if (cursor != lexer->cursor)
	{
	    cursor = lexer->cursor;
	    fsetpos(stream, &cursor);
	}
	char result = fgetc(stream);
	fseek(stream, -sizeof(char), SEEK_CUR); /* Go back to cursor */
	return result;
    }
    
    case ATOM_LEXER_STRING:
    {
	return lexer->buffer.string[lexer->cursor];
    }
    
    default:
	assert(0 && "Type of lexer (lexer->type) is invalid");
    }
}


/**
 * Goto next position and peek the char
 */
static char atom_lexer_next(atom_lexer_t* lexer)
{
    assert(lexer != NULL);
    lexer->cursor++;
    char c = atom_lexer_peek(lexer);
    if (c == '\n')
    {
	lexer->line++;
	lexer->column  = 1;
    }
    else
    {
	lexer->column += 1;
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
static void atom_lexer_skipspace(atom_lexer_t* lexer)
{
    assert(lexer != NULL);

    char c = atom_lexer_peek(lexer);
    while (atom_isspace(c))
    {
	c = atom_lexer_next(lexer);
    }
}


/**
 * Skip comment
 */
static void atom_lexer_skipcomment(atom_lexer_t* lexer)
{
    assert(lexer != NULL);

    char c = atom_lexer_peek(lexer);
    while (c && c != '\n' && c != '\r')
    {
	c = atom_lexer_next(lexer);
    }
    atom_lexer_next(lexer);
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
static void _atom_lexer_error(atom_lexer_t* lexer, int errcode)
{
    assert(lexer != NULL);
  
    int line   = lexer->line;
    int column = lexer->column;
    int cursor = lexer->errcursor = lexer->cursor;
    int c      = atom_lexer_peek(lexer);
    switch ((lexer->errcode = errcode))
    {
    case ATOM_ERROR_UNBALANCED:
	fprintf(stderr,
		"[%d:%d:%d]: Unbalance '%c'\n", line, column, cursor, c);
	break;
    
    case ATOM_ERROR_UNEXPECTED:
	fprintf(stderr,
		"[%d:%d:%d]: Unexpected '%c'\n", line, column, cursor, c);
	break;

    case ATOM_ERROR_UNTERMINATED:
	fprintf(stderr,
		"[%d:%d:%d]: Unterminated '%c'\n", line, column, cursor, c);
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


/* @function: atom_create
 */
atom_node_t* atom_create(atom_type_t type, atom_text_t name)
{
    atom_node_t* node = atom_newnode();
    if (!node)
    {
	/* @error: Out of memory
	 */
	return NULL;
    }
    node->type      = type;
    node->name      = name;
    node->parent    = NULL;
    node->children  = NULL;
    node->lastchild = NULL;
    node->next      = NULL;
    node->prev      = NULL;
    return node;
}


/* @function: atom_delete
 */
void atom_delete(atom_node_t* node)
{
    if (node)
    {
	/* Remove from parent
	 */
	if (node->prev)
	{
	    node->prev = node->next;
	}
	if (node->next)
	{
	    node->next = node->prev;
	}
	if (node->parent)
	{
	    if (node->parent->children == node)
	    {
		node->parent->children = node->next;
	    }
	    if (node->parent->lastchild == node)
	    {
		node->parent->lastchild = node->prev;
	    }
	}
    
	/* Remove children
	 */
	if (node->type == ATOM_LIST)
	{
	    while (node->children)
	    {
		atom_node_t* next = node->children->next;
		atom_delete(node->children);
		node->children = next;
	    }
	}

	/* Free usage heap memory
	 */
	atom_freenode(node);
    }
}


/* @function: atom_tolong
 */
atom_bool_t atom_tolong(const char* text, atom_data_t* value)
{
    assert(text != NULL && value != NULL);

    const char* ptr = text;
    char c    = *ptr;
    int  sign = 1;
    switch (c)
    {
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

    value->as_long = 0;
    while ((c = *ptr++))
    {
	if (!atom_isdigit(c))
	{
	    return ATOM_FALSE;
	}
	value->as_long = value->as_long * 10 + (c - '0');
    }
    value->as_long *= sign;
    return ATOM_TRUE;
}


/* @function: atom_toreal
 */ 
atom_bool_t atom_toreal(const char* text, atom_data_t* value)
{
    assert(text != NULL && value != NULL);
  
    const char* ptr = text;
    char c    = *ptr;
    int  sign = 1;
    switch (c)
    {
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

    atom_bool_t dotfound  = ATOM_FALSE;
    atom_real_t precision = 10;
    value->as_real        = 0.0;
    while ((c = *ptr++))
    {
	if (c == '.')
	{
	    if (dotfound)
	    {
		return ATOM_FALSE;
	    }
	    dotfound = ATOM_TRUE;
	    continue;
	}
	else if (!atom_isdigit(c))
	{
	    return ATOM_FALSE;
	}
	
	if (dotfound)
	{
	    value->as_real = value->as_real + (c - '0') / precision;
	    precision     *= 10;
	}
	else
	{
	    value->as_real = value->as_real * 10 + (c - '0');
	}
    }
    value->as_real *= sign;
    return ATOM_TRUE;
}

static atom_node_t* atom_read(atom_lexer_t* lexer);
/**
 * Read an atom token
 */
static atom_node_t* atom_readatom(atom_lexer_t* lexer)
{
    assert(lexer != NULL);

    /* Nothing to parse when its end
     */
    atom_lexer_skipspace(lexer);
    if (atom_lexer_iseof(lexer))
    {
	return NULL;
    }
  
    char c    = atom_lexer_peek(lexer);
    int  head = lexer->cursor;
    int  tail = head;
    atom_node_t* node = NULL;
    if (c == '"')
    {
	/* Get character until terminated
	 */
	c = atom_lexer_next(lexer);
	head++;
	while (c && c != '"')
	{
	    tail++;
	    c = atom_lexer_next(lexer);
	}

	/* A valid text must terminated with '"'
	 */
	if (c != '"')
	{
	    atom_lexer_error(lexer, ATOM_ERROR_UNTERMINATED);
	    return NULL;
	}

	/* Do the last job to finish read a text
	 */
	atom_lexer_next(lexer);
	atom_text_t text;
	text.head = head;
	text.tail = tail;
	node = atom_newtext(ATOM_TEXT_NULL, text);
    }
    else
    {
	char  text[1024];
	char* ptr = text;
	/* Read sequence characters still meet a separator
	 */
	while (c && !atom_isspace(c) && !atom_ispunct(c))
	{
	    *ptr++ = c; tail++;
	    c = atom_lexer_next(lexer);
	}
	*ptr++ = 0;

	/* Parsing value of the token
	 */
	atom_data_t value;
	if (atom_tolong(text, &value))
	{
	    node = atom_newlong(ATOM_TEXT_NULL, value.as_long);
	}
	else if (atom_toreal(text, &value))
	{
	    node = atom_newreal(ATOM_TEXT_NULL, value.as_real);
	}
	else
	{
	    atom_text_t text;
	    text.head = head;
	    text.tail = tail;
	    node = atom_create(ATOM_NAME, text);
	}
    }

    /* Read without error
     */
    return node;
}


/**
 *
 */
static atom_bool_t atom_list_to_single(atom_node_t* node)
{
    atom_node_t* children  = node->children;
    atom_node_t* lastchild = node->lastchild;
    if (children && children == lastchild)
    {
	node->type = children->type;
	node->data = children->data;
	atom_delete(children);
	node->children = node->lastchild = NULL;
	return ATOM_TRUE;
    }
    return ATOM_FALSE;
}


/**
 * Read list
 */
static atom_node_t* atom_readlist(atom_lexer_t* lexer)
{
    assert(lexer != NULL);

    if (atom_lexer_iseof(lexer))
    {
	return NULL;
    }

    char c = atom_lexer_peek(lexer);
    char close;
    switch (c)
    {
    case '(': close = ')'; break;
    case '[': close = ']'; break;
    case '{': close = '}'; break;
    default:
	atom_lexer_error(lexer, ATOM_ERROR_UNEXPECTED);
	break;
    }

    c = atom_lexer_next(lexer);
    atom_node_t* root = NULL;
    atom_node_t* node = NULL;
    do {
	/* Read child token, only parse when token is valid
	 */
	atom_lexer_skipspace(lexer);
	if ((node = atom_read(lexer)))
	{
	    if (!root)
	    {
		root = node;
		if (root->type == ATOM_NAME)
		{
		    root->type         = ATOM_LIST;
		    root->data.is_root = ATOM_TRUE;
		}
	    }
	    else
	    {
		if (root->type != ATOM_LIST || !root->data.is_root)
		{
		    atom_node_t* list  = atom_newlist(ATOM_TEXT_NULL);
		    list->data.is_root = ATOM_TRUE;
		    atom_addchild(list, root);
		    root = list;
		}
		if (node->type == ATOM_NAME)
		{
		    atom_lexer_error(lexer, ATOM_ERROR_UNEXPECTED);
		    atom_delete(node);
		    atom_delete(root);
		    return NULL;
		}
		atom_addchild(root, node);
	    }
	}

	/* Get next character to check if there is an end sign
	 */
	atom_lexer_skipspace(lexer);
	c = atom_lexer_peek(lexer);
    } while (c && c != close && node);

    /* Must be end with ${close} character
     */
    if (c != close)
    {
	atom_delete(root);
	atom_lexer_error(lexer, ATOM_ERROR_UNBALANCED);
	return NULL;
    }
    atom_lexer_next(lexer);
    atom_list_to_single(root);
    return root;
}


/**
 * Read a single token
 */
atom_node_t* atom_read(atom_lexer_t* lexer)
{
    assert(lexer != NULL);

    /* Leave 
     */
    atom_lexer_skipspace(lexer);
    if (atom_lexer_iseof(lexer))
    {
	return NULL;
    }

    char c = atom_lexer_peek(lexer);
    switch (c)
    {
    case ';':
	atom_lexer_skipcomment(lexer);
	return atom_read(lexer);
      
    case '(':
    case '[':
    case '{':
	return atom_readlist(lexer);
      
    case ')':
    case ']':
    case '}':
	atom_lexer_error(lexer, ATOM_ERROR_UNEXPECTED);
	return NULL;

    default:
	return atom_readatom(lexer);
    }
  
    return NULL;
}


/**
 * Parse lexer data to atom
 */
atom_node_t* atom_parse(atom_lexer_t* lexer)
{
    assert(lexer != NULL);
  
    atom_node_t* root = NULL;
    atom_node_t* node = NULL;
    atom_lexer_skipspace(lexer);
    while (!atom_lexer_iseof(lexer))
    {
	node = atom_read(lexer);

	/* NULL pointer handling
	 */
	if (!node)
	{
	    atom_lexer_skipspace(lexer);
	    if (lexer->errcode != ATOM_ERROR_NONE)
	    {
		atom_delete(node);
		return NULL;
	    }
	    if (atom_lexer_iseof(lexer))
	    {
		break;
	    }
	    continue;
	}
    
	/* Append to root list or make root if root is not setted
	 */
	if (!root)
	{
	    root = node;
	}
	else
	{
	    if (root->type != ATOM_LIST || !root->data.is_root)
	    {
		atom_node_t* list  = atom_newlist(ATOM_TEXT_NULL);
		list->data.is_root = ATOM_TRUE;
		atom_addchild(list, root);
		root = list;
	    }
	    atom_addchild(root, node);
	}
    }
    atom_list_to_single(root);
    return root;
}


/* @function: atom_totext
 */
static size_t atom_totext(atom_lexer_t* lexer, atom_node_t* node,
			  char* text, size_t size)
{
    assert(node != NULL);
    assert(text != NULL && size > 0);


    static int stack = 0;
    size_t result = 0;
    for (int i = 0; i < stack * 2; i++)
    {
	*text++ = ' ';
	result++;
    }
  
    if (node->type == ATOM_LIST)
    {
	stack++;
	char* tptr = text;
	atom_node_t* child = node->children;
	/* Write name to text
	 */
	//if (!atomIsRoot(node) || !node->name)
	*tptr++ = '(';
	result++;
	if (!atom_istextnull(node->name))
	{
	    /* Open list with '(' character
	     * We not use '[' or '{', but it's still valid in using
	     * and hand-edit
	     */
	    atom_text_t name = node->name;
	    for (int i = name.head; i <= name.tail; i++)
	    {
		*tptr++ = atom_lexer_get(lexer, i);
		result++;
	    }
	    *tptr++ = ' ';
	    result++;
	}

	/* Write children values
	 */
	while (child)
	{
	    *tptr++ = '\n';
	    result++;
	    /* Get child value
	     */
	    size_t count = atom_totext(lexer, child, tptr, size - result);
	    child   = child->next;
	    tptr   += count;
	    result += count;
	    if (child)
	    {
		*tptr++ = ' '; /* Must have a separator */
		result++;
	    }
	}
	/* Close list
	 */ 
	//if (!atomIsRoot(node) || !node->name)
	*tptr++ = ')';
	result++;
	stack--;
    }
    else
    {
	if (!atom_istextnull(node->name))
	{
	    *text++ = '(';
	    result++;
	    for (int i = node->name.head; i <= node->name.tail; i++)
	    {
		result++;
		*text++ = atom_lexer_get(lexer, i);
	    }
	    *text++ = ' ';
	    result++;
	}
	
	switch (node->type)
	{
	case ATOM_LONG:
	{
	    size_t count = sprintf(text, "%ld", node->data.as_long);
	    result += count;
	    text   += count;
	} break;
	
	case ATOM_REAL:
	{
	    size_t count = sprintf(text, "%lf", node->data.as_real);
	    result += count;
	    text   += count;
	} break;

	case ATOM_TEXT:
	    result++;
	    *text++ = '\"';
	    for (int i = node->data.as_text.head,
		     n = node->data.as_text.tail;
		 i <= n; i++)
	    {
		result++;
		*text++ = atom_lexer_get(lexer, i);
	    }
	    result++;
	    *text++ = '\"';
	    break;
	    
	default:
	    break;
	}
	
	if (!atom_istextnull(node->name))
	{
	    *text++ = ')';
	    result++;
	}
    }
  return result;
}


/* @function: atom_save_file
 */
int atom_save_file(atom_node_t* node, FILE* file)
{
    assert(node != NULL);
    assert(file != NULL);
    return ATOM_ERROR_NONE;
}


/* @function: atom_save_text
 */
int atom_save_text(atom_lexer_t* lexer, atom_node_t* node,
		  char* buffer, size_t size)
{
    assert(lexer != NULL);
    if (!node || !buffer || size == 0)
    {
	return ATOM_ERROR_ARGUMENTS;
    }
    
    assert(buffer != NULL && size > 0);
    size_t count = atom_totext(lexer, node, buffer, size);
    buffer[count] = 0;
    return ATOM_ERROR_NONE;
}

/* @function: atomAddChild
 */
void atom_addchild(atom_node_t* node, atom_node_t* child)
{
    assert(node != NULL);
    child->parent = node;
    
    if (child->type == ATOM_LIST)
    {
	child->data.is_root = ATOM_FALSE;
    }
    
    if (node->lastchild)
    {
	child->prev     = node->lastchild; 
	node->lastchild = node->lastchild->next = child;
    }
    else
    {
	node->lastchild = node->children        = child;
    }
}


/* @function: atom_print
 */
void atom_print(atom_lexer_t* lexer, atom_node_t* node)
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
    if (node)
    {
	for (int i = 0; i < stack; i++)
	{
	    fputc(' ', stdout);
	}
	
	fputs(types[node->type], stdout);
	fputs(" - ", stdout);
	for (int i = node->name.head; i <= node->name.tail; i++)
	{
	    fputc(atom_lexer_get(lexer, i), stdout);
	}
	
	switch (node->type)
	{
	case ATOM_LIST:
	    stack++;
	    atom_node_t* children = node->children;
	    if (children == NULL)
	    {
		fputs(" - (null)\n", stdout);
	    }
	    else
	    {
		fputc('\n', stdout);
		while (children)
		{
		    atom_print(lexer, children);
		    children = children->next;
		}
	    }
	    stack--;
	    break;
      
	case ATOM_LONG:
	    printf(" - %ld\n", node->data.as_long);
	    break;
      
	case ATOM_REAL:
	    printf(" - %lf\n", node->data.as_real);
	    break;
      
	case ATOM_TEXT:
	    fputs(" - \"", stdout);
	    for (int i = node->data.as_text.head,
		     n = node->data.as_text.tail;
		 i <= n; i++)
	    {
		fputc(atom_lexer_get(lexer, i), stdout);
	    }
	    fputs("\"\n", stdout);
	    break;

	default:
	    fputc('\n', stdout);
	    break;
	}
    }
}


/* @function: atom_textcpy
 */
size_t atom_textcpy(atom_lexer_t* lexer, atom_text_t text,
		    char* string)
{
    assert(lexer != NULL);
  
    size_t size = 0;
    for (int i = text.head; i < text.tail; i++, size++)
    {
	*string++ = atom_lexer_get(lexer, i);
    }
    return size;
}


/* @function: atom_textcmp
 */
size_t atom_textcmp(atom_lexer_t* lexer, atom_text_t text, const char* string)
{
    assert(lexer != NULL);
  
    char a, b;
    for (int i = text.head; i < text.tail && (b = *string); i++, string++)
    {
	a = atom_lexer_get(lexer, i);
	if (a < b)
	{
	    return -1;
	}
	else if (a > b)
	{
	    return 1;
	}
    }
    return 0;
}
