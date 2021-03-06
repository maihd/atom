/**
 * Atom - file data format with s-expression
 *
 * @author: MaiHD
 * @license: Free to use
 * @copyright: MaiHD @ ${HOME}, 2017 - 2018
 */

#ifndef __ATOM_H__
#define __ATOM_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/******
 * Metadata constants
 */
#define ATOM_LIBNAME "libatom"
#define ATOM_VERSION "v1.0.04"

/******
 * Custom modifiers
 * Predefine before include this file for customizing
 */
#ifndef __atomextern
#define __atomextern extern
#endif

#ifndef __atominline
# ifdef __GNUC__
#  define __atominline __attribute__((always_inline))
# elif  defined(_MSC_VER)
#  define __atominline __forceinline
# elif  defined(__cplusplus)
#  define __atominline static inline
# else
#  define __atominline static
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* BEGIN OF EXTERN "C" */

/******
 * Types definition
 */
typedef char    atom_bool_t; /* Boolean         */
typedef int64_t atom_long_t; /* Integer number  */
typedef double  atom_real_t; /* Real    number  */
typedef union                /* A text in lexer */
{
    struct
    {
        int head;
        int tail;
    };
    const char* cstr;
} atom_text_t;

enum
{
    ATOM_TRUE  = 1,
    ATOM_FALSE = 0,
};

/**
 * Atom value type
 * @note: commented-line is the feature in decision
 */
typedef enum
{
    ATOM_NONE = 0, /* Error data type */
    ATOM_LIST,
    ATOM_LONG,   
    ATOM_REAL,
    ATOM_TEXT,
    ATOM_NAME,     /* The first node of list, after parse will be the name */
} atom_type_t;


/**
 * Error code
 */
enum
{
    ATOM_ERROR_NONE          =  0,
    ATOM_ERROR_ARGUMENTS     = -1,
    ATOM_ERROR_LEXERTYPE     = -2,
    ATOM_ERROR_UNBALANCED    = -3,
    ATOM_ERROR_UNEXPECTED    = -4,
    ATOM_ERROR_UNTERMINATED  = -5,
};


/**
 * Lexer type
 */
enum
{
    ATOM_LEXER_STRING,
    ATOM_LEXER_STREAM,
};


/**
 * Atom immediately present value
 * @size: fixed-size, runtime dependent (4bytes on 32bits, 8bytes on 64bits)
 * @note: commented-line is the feature in decision
 */
typedef union
{
    atom_bool_t is_root; /* Available when is list */
    atom_long_t as_long;
    atom_real_t as_real;
    atom_text_t as_text;
} atom_data_t;


/**
 * A node have 2 form: atom and list
 * Atom we just need to present value
 * List is more complicate than that, list can contain child nodes
 * @note: commented-line is the feature in decision
 */
typedef struct atom_node atom_node_t;
struct atom_node
{
    atom_type_t  type;
    atom_text_t  name;
    atom_data_t  data;
    atom_node_t* prev;
    atom_node_t* next;
    atom_node_t* parent;
    atom_node_t* children;
    atom_node_t* lastchild;

    /* No padding needed */
};

/**
 * Atom lexer for parsing
 */
typedef struct
{
    int    type;
    int    line;
    int    column;
    int    cursor;        /* Cursor position  */
    size_t length;        /* Length of buffer */
    union
    {
        const char* string;
        FILE*       stream;
    };

    int    errcode;       /* Error code        */
    int    errcursor;     /* Position at error */

    /* No padding needed */
} atom_lexer_t;

/**
 * Global constants
 */
static const atom_text_t ATOM_TEXT_NULL = { 0, 0 };


/**
 * Initialize memory buffer of the atom's runtime
 *
 * @param data    - pointer to the memory
 * @param size    - size of the memory
 * @param extract - function to extract the memory from the buffer
 * @param collect - function to collect the memory to the buffer
 * @return none
 */
__atomextern void atom_init(void* data, size_t size, void* (*extract)(void*, size_t), void (*collect)(void*, void*));

/**
 * Release all usage memories by the atom's runtime
 */
__atomextern void atom_release(void);

/**
 * Initialize lexer with context
 * @params type    - Type of lexer (ATOM_LEXER_STREAM, ATOM_LEXER_STRING)
 * @params context - const char* or FILE*
 */
__atomextern int atom_lexer_init(atom_lexer_t*, int type, void* context);
__atomextern int atom_lexer_free(atom_lexer_t*);

__atomextern atom_node_t* atom_create(atom_type_t type, atom_text_t name);
__atomextern void         atom_delete(atom_node_t* node);

__atomextern void         atom_addchild(atom_node_t* node, atom_node_t* child);
//__atomextern void         atom_remove_child(atom_node_t* node, atom_node_t* child);

__atomextern atom_node_t* atom_parse(atom_lexer_t* lexer);

__atomextern int atom_save_stream(atom_node_t* node, FILE* stream);
__atomextern int atom_save_string(atom_node_t* node, char* string, size_t length);
__atomextern int atom_save_stream_with_lexer(atom_lexer_t* lexer, atom_node_t* node, FILE* stream);  
__atomextern int atom_save_string_with_lexer(atom_lexer_t* lexer, atom_node_t* node, char* string, size_t length);

__atominline atom_node_t* atom_newlist(atom_text_t name);
__atominline atom_node_t* atom_newlong(atom_text_t name, atom_long_t value);
__atominline atom_node_t* atom_newreal(atom_text_t name, atom_real_t value);
__atominline atom_node_t* atom_newtext(atom_text_t name, atom_text_t value);

/******
 * Utilities
 */

__atominline atom_bool_t  atom_istextnull(atom_text_t text);
__atomextern atom_bool_t  atom_tolong(const char* text, atom_data_t* value);
__atomextern atom_bool_t  atom_toreal(const char* text, atom_data_t* value);
__atomextern size_t       atom_textcpy(atom_lexer_t* lexer, atom_text_t text, char* buffer);
__atomextern size_t       atom_textcmp(atom_lexer_t* lexer, atom_text_t text, const char* string);

__atomextern size_t       atom_getfilesize(FILE* file);
__atomextern void         atom_print(atom_lexer_t* lexer, atom_node_t* node);

/******
 * Inline functions definition
 */


__atominline atom_bool_t atom_istextnull(atom_text_t text)
{
    return (text.head <= 0 && text.tail <= 0);
}

/* @function: atom_newlist
 */
__atominline atom_node_t* atom_newlist(atom_text_t name)
{
    atom_node_t* node  = atom_create(ATOM_LIST, name);
    node->data.is_root = ATOM_FALSE;
    return node;
}


/* @function: atom_newlong
 */
__atominline atom_node_t* atom_newlong(atom_text_t name, atom_long_t value)
{
    atom_node_t* node  = atom_create(ATOM_LONG, name);
    node->data.as_long = value;
    return node;
}


/* @function: atom_newreal
 */
__atominline atom_node_t* atom_newreal(atom_text_t name, atom_real_t value)
{
    atom_node_t* node  = atom_create(ATOM_REAL, name);
    node->data.as_real = value;
    return node;
}


/* @function: atom_newtext
 */
__atominline atom_node_t* atom_newtext(atom_text_t name, atom_text_t value)
{
    atom_node_t* node  = atom_create(ATOM_TEXT, name);
    node->data.as_text = value;
    return node;
}


#ifdef ATOM_IMPL
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <setjmp.h> 

/***********************
* Configurable helper
***********************/
#ifndef atom_assert                             
#include <assert.h>
#define atom_assert(exp, msg, ...) assert(exp)
#endif

#define __STR__(x) __VAL__(x)
#define __VAL__(x) #x

#ifdef NDEBUG
#define atom_lexer_error(l, e) _atomLexerError(l, e)
#else
#define atom_lexer_error(l, e)						                             \
  fprintf(stderr, "Error at:" __FILE__ ":" __STR__(__LINE__) ":%s\n", __func__); \
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
 * Internal, default malloc function
 */
static void* atom_malloc(void* data, size_t size)
{
    (void)data;
    return malloc(size);
}

/** 
 * Internal, default free function
 */
static void atom_free(void* data, void* pointer)
{
    (void)data;
    free(pointer);
}

/**
 * Atom node memory pool
 */
typedef struct atom_nodepool atom_nodepool_t;
struct atom_nodepool
{
    atom_nodepool_t* prev;
    atom_node_t*     node;  
    /* No padding needed */
} *atom_nodepool = NULL; 

/**
 * Memory buffer
 */
struct
{
    void*  data;
    size_t size;
    void*  (*extract)(void* data, size_t size);
    void   (*collect)(void* data, void* pointer); 
    /* No padding needed */
} atom_membuf = { NULL, 0, atom_malloc, atom_free };

/**
* Allocate node memory, getting from pool or craete new
* @function: atom_newnode
*/
static atom_node_t* atom_newnode(void)
{
    if (!atom_nodepool || !atom_nodepool->node)
    {
        const size_t size = sizeof(atom_nodepool_t) + sizeof(atom_node_t) * ATOM_BUCKETS;

        atom_nodepool_t* nodepool = atom_membuf.extract(atom_membuf.data, size);
        if (!nodepool)
        {
            /* @error: out of memory */
            return NULL;
        }

        atom_node_t* node = (atom_node_t*)((char*)nodepool + sizeof(atom_nodepool_t));
        nodepool->node    = node; /* head node */
        for (int i = 0; i < ATOM_BUCKETS - 1; i++)
        {
            node->next = node + i;
        }
        node->next = NULL; /* tail node */

        nodepool->prev = atom_nodepool;
        atom_nodepool  = nodepool;
    }

    atom_node_t*   node = atom_nodepool->node;
    atom_nodepool->node = node->next;
    return node;
}


/**
* Free node
*/
static void atom_freenode(atom_node_t* node)
{
    if (atom_nodepool && node)
    {
        node->next          = atom_nodepool->node;
        atom_nodepool->node = node;
    }
}

/* @function: atom_init */
void atom_init(void* data, size_t size, void* (*extract)(void*, size_t), void (*collect)(void*, void*))
{
    atom_membuf.data    = data;
    atom_membuf.size    = size;
    atom_membuf.extract = extract;
    atom_membuf.collect = collect;
}

/* @function: atom_release */
void atom_release(void)
{
    while (atom_nodepool)
    {
        atom_membuf.collect(atom_membuf.data, atom_nodepool);
        atom_nodepool = atom_nodepool->prev;
    }
}


/* @function: atom_getfilesize */
size_t atom_getfilesize(FILE* file)
{
    atom_assert(file != NULL);
    fpos_t prev = ftell(file);
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fsetpos(file, &prev);
    return size;
}


/* @function: atom_lexer_init */
int atom_lexer_init(atom_lexer_t* lexer, int type, void* context)
{
    atom_assert(lexer != NULL);

    if (!context)
    {
        return ATOM_ERROR_ARGUMENTS;
    }

    /* Set context depend on lexer type
    */
    switch (type)
    {
    case ATOM_LEXER_STREAM:
        lexer->length = atom_getfilesize(context);
        lexer->stream = context;
        break;

    case ATOM_LEXER_STRING:
        lexer->length = strlen(context);
        lexer->string = context;
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

int atom_lexer_free(atom_lexer_t* lexer)
{                         
    if (lexer)
    {
        // TODO: do release memory
    }
    return ATOM_ERROR_NONE;
}


/**
* Check if lexer reach the end
* We don't use inline modifier 
* 'cause may has more process to do with
*/
static atom_bool_t atom_lexer_iseof(atom_lexer_t* lexer)
{
    atom_assert(lexer != NULL);
    return lexer->cursor >= lexer->length;
}



/**
* Get the char at the cursor position
*/
static char atom_lexer_get(atom_lexer_t* lexer, int cursor)
{
    atom_assert(lexer != NULL);
    switch (lexer->type)
    {
    case ATOM_LEXER_STREAM:
    {
        FILE*  stream = lexer->stream;
        fpos_t pos = cursor;
        fsetpos(stream, &pos);
        return fgetc(stream);
    }

    case ATOM_LEXER_STRING:
    {
        return lexer->string[cursor];
    }

    default:
        atom_assert(0 && "Type of lexer (lexer->type) is invalid");
    }
}


/**
* Get the char at the cursor position
*/
static char atom_lexer_peek(atom_lexer_t* lexer)
{
    atom_assert(lexer != NULL);

    if (atom_lexer_iseof(lexer))
    {
        return 0;
    }

    switch (lexer->type)
    {
    case ATOM_LEXER_STREAM:
    {
        FILE*  stream = lexer->stream;
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
        return lexer->string[lexer->cursor];
    }

    default:
        atom_assert(0, "Type of lexer (lexer->type) is invalid");
    }
}


/**
* Goto next position and peek the char
*/
static char atom_lexer_next(atom_lexer_t* lexer)
{
    atom_assert(lexer != NULL);
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
* Skip space
*/
static void atom_lexer_skipspace(atom_lexer_t* lexer)
{
    atom_assert(lexer != NULL);

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
    atom_assert(lexer != NULL);

    char c = atom_lexer_peek(lexer);
    while (c && c != '\n' && c != '\r')
    {
        c = atom_lexer_next(lexer);
    }
    atom_lexer_next(lexer);
}

/**
* Raise an error, jump back to caller with error code 
*/
static void _atom_lexer_error(atom_lexer_t* lexer, int errcode)
{
    atom_assert(lexer != NULL);

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
    atom_assert(text != NULL && value != NULL);

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
    atom_assert(text != NULL && value != NULL);

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
    atom_assert(lexer != NULL);

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
        text.tail = tail + 1;
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
    if (node)
    {
        atom_node_t* children = node->children;
        atom_node_t* lastchild = node->lastchild;
        if (children && children == lastchild)
        {
            node->type = children->type;
            node->data = children->data;
            atom_delete(children);
            node->children = node->lastchild = NULL;
            return ATOM_TRUE;
        }
    }
    return ATOM_FALSE;
}


/**
* Read list
*/
static atom_node_t* atom_readlist(atom_lexer_t* lexer)
{
    atom_assert(lexer != NULL);

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
    atom_assert(lexer != NULL);

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
    atom_assert(lexer != NULL);

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

static size_t atom_totext(atom_node_t* node, char* text, size_t size)
{
    atom_assert(node != NULL);
    atom_assert(text != NULL && size > 0);

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
        *tptr++ = '(';
        result++;

        if (node->name.cstr)
        {
            /* Open list with '(' character
            * We not use '[' or '{', but it's still valid in using
            * and hand-edit
            */
            int count = (int)(strcpy(tptr, node->name.cstr) - tptr); 
            tptr   += count;
            *tptr++ = ' ';
            result += count;
        }

        /* Write children values
        */
        while (child)
        {
            *tptr++ = '\n';
            result++;
            /* Get child value
            */
            size_t count = atom_totext(child, tptr, size - result);
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
        *tptr++ = ')';
        result++;
        stack--;
    }
    else
    {
        if (node->name.cstr)
        {
            *text++ = '(';
            result++;      

            int count = (int)(strcpy(text, node->name.cstr) - text);
            text   += count;
            *text++ = ' ';
            result += count;

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

            int count = (int)(strcpy(text, node->data.as_text.cstr) - text);
            text   += count;
            result += count;
            
            result++;
            *text++ = '\"';
            break;

        default:
            break;
        }

        if (node->name.cstr)
        {
            *text++ = ')';
            result++;
        }
    }
    return result;
}

/* @function: atom_totext_with_lexer */
static size_t atom_totext_with_lexer(atom_lexer_t* lexer, atom_node_t* node, char* text, size_t size)
{
    atom_assert(node != NULL);
    atom_assert(text != NULL && size > 0);


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
            size_t count = atom_totext_with_lexer(lexer, child, tptr, size - result);
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


/* @function: atom_save_stream */
int atom_save_stream(atom_node_t* node, FILE* stream)
{
    atom_assert(node != NULL);
    atom_assert(stream != NULL);

    static int stack = 0;
    int result = 0;
    for (int i = 0; i < stack; i += 2)
    {
        fputc(' ', stream);
        fputc(' ', stream);
        result += 2;
    }

    if (node->type == ATOM_LIST)
    {
        stack++;
        atom_node_t* child = node->children;
        /* Write name to text
        */
        fputc('(', stream);
        result++;
        if (node->name.cstr)
        {
            /* Open list with '(' character
            * We not use '[' or '{', but it's still valid in using
            * and hand-edit
            */
            result += fputs(node->name.cstr, stream); 
            fputc(' ', stream);
            result++;
        }

        /* Write children values
        */
        while (child)
        {
            fputc('\n', stream);
            result++;
            /* Get child value
            */
            int count = atom_save_stream(child, stream);
            child   = child->next;
            result += count;
            if (child)
            {
                fputc(' ', stream);
                result++;
            }
        }
        /* Close list
        */ 
        fputc(')', stream);
        result++;
        stack--;
    }
    else
    {
        if (node->name.cstr)
        {
            fputc('(', stream);
            result++; 
            result += fputs(node->name.cstr, stream);
            fputc(' ', stream);
            result++;
        }

        switch (node->type)
        {
        case ATOM_LONG:
        {
            size_t count = fprintf(stream, "%ld", node->data.as_long);
            result += count;
        } break;

        case ATOM_REAL:
        {
            size_t count = fprintf(stream, "%lf", node->data.as_real);
            result += count;
        } break;

        case ATOM_TEXT:
            result++;
            fputc('\"', stream);
            result += fputs(node->data.as_text.cstr, stream) + 1;
            fputc('\"', stream);
            break;

        default:
            break;
        }

        if (node->name.cstr)
        {              
            fputc(')', stream);
            result++;
        }
    }

    return result;
}

/* @function: atom_save_stream_with_lexer */
int atom_save_stream_with_lexer(atom_lexer_t* lexer, atom_node_t* node, FILE* stream)
{
    atom_assert(node != NULL);
    atom_assert(stream != NULL);

    static int stack = 0;
    int result = 0;
    for (int i = 0; i < stack; i += 2)
    {
        fputc(' ', stream);
        fputc(' ', stream);
        result += 2;
    }

    if (node->type == ATOM_LIST)
    {
        stack++;
        atom_node_t* child = node->children;
        /* Write name to text
        */
        fputc('(', stream);
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
                fputc(atom_lexer_get(lexer, i), stream);
                result++;
            }
            fputc(' ', stream);
            result++;
        }

        /* Write children values
        */
        while (child)
        {
            fputc('\n', stream);
            result++;
            /* Get child value
            */
            int count = atom_save_stream(child, stream);
            child   = child->next;
            result += count;
            if (child)
            {
                fputc(' ', stream);
                result++;
            }
        }
        /* Close list
        */ 
        fputc(')', stream);
        result++;
        stack--;
    }
    else
    {
        if (!atom_istextnull(node->name))
        {
            fputc('(', stream);
            result++;                               
            atom_text_t name = node->name;
            for (int i = name.head; i <= name.tail; i++)
            {
                fputc(atom_lexer_get(lexer, i), stream);
                result++;
            }
            fputc(' ', stream);
            result++;
        }

        switch (node->type)
        {
        case ATOM_LONG:
        {
            size_t count = fprintf(stream, "%ld", node->data.as_long);
            result += count;
        } break;

        case ATOM_REAL:
        {
            size_t count = fprintf(stream, "%lf", node->data.as_real);
            result += count;
        } break;

        case ATOM_TEXT:
            result++;
            fputc('\"', stream);
            
            atom_text_t text = node->data.as_text;
            for (int i = text.head; i <= text.tail; i++)
            {
                fputc(atom_lexer_get(lexer, i), stream);
                result++;
            }

            result++;
            fputc('\"', stream);
            break;

        default:
            break;
        }

        if (!atom_istextnull(node->name))
        {              
            fputc(')', stream);
            result++;
        }
    }

    return result;
}

/* @function: atom_save_string */
int atom_save_string(atom_node_t* node, char* string, size_t length)
{
    if (!node || !string || length == 0)
    {
        return ATOM_ERROR_ARGUMENTS;
    }

    atom_assert(string != NULL && length > 0);
    size_t count = atom_totext(node, string, length);
    string[count] = 0;
    return ATOM_ERROR_NONE;
}

/* @function: atom_save_string_with_lexer */
int atom_save_string_with_lexer(atom_lexer_t* lexer, atom_node_t* node, char* string, size_t length)
{
    atom_assert(lexer != NULL);

    if (!node || !string || length == 0)
    {
        return ATOM_ERROR_ARGUMENTS;
    }

    atom_assert(string != NULL && length > 0);
    size_t count = atom_totext_with_lexer(lexer, node, string, length);
    string[count] = 0;
    return ATOM_ERROR_NONE;
}

/* @function: atomAddChild
*/
void atom_addchild(atom_node_t* node, atom_node_t* child)
{
    atom_assert(node != NULL);
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
size_t atom_textcpy(atom_lexer_t* lexer, atom_text_t text, char* string)
{
    atom_assert(lexer != NULL);

    size_t size = 0;
    for (int i = text.head, n = text.tail; i < n; i++, size++)
    {
        *string++ = atom_lexer_get(lexer, i);
    }
    *string++ = 0;
    return size;
}


/* @function: atom_textcmp
*/
size_t atom_textcmp(atom_lexer_t* lexer, atom_text_t text, const char* string)
{
    atom_assert(lexer != NULL);

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

#endif 

/* END OF EXTERN "C" */
#ifdef __cplusplus
}
#endif

#endif /* __ATOM_H__ */


