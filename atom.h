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

/******
 * Metadata constants
 */
#define ATOM_LIBNAME "libatom"
#define ATOM_VERSION "v1.0.01"

/******
 * Custom modifiers
 * Predefine before include this file for customizing
 */
#ifndef __atomextern
#define __atomextern extern
#endif
#ifndef __atominline
#define __atominline static inline
#endif

/******
 * Types definition
 */
typedef char    atom_bool_t; /* Boolean         */
typedef int64_t atom_long_t; /* Integer number  */
typedef double  atom_real_t; /* Real    number  */
typedef struct               /* A text in lexer */
{
    int head;
    int tail;
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
    ATOM_ERROR_NONE,
    ATOM_ERROR_ARGUMENTS,
    ATOM_ERROR_LEXERTYPE,
    ATOM_ERROR_UNBALANCED,
    ATOM_ERROR_UNEXPECTED,
    ATOM_ERROR_UNTERMINATED,
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
    } buffer;

    int    errcode;       /* Error code        */
    int    errcursor;     /* Position at error */
  
    /* No padding needed */
} atom_lexer_t;

/**
 * Global constants
 */
static const atom_text_t ATOM_TEXT_NULL = { 0, 0 };


/**
 * Release all usage memories by atom module
 */
__atomextern void atom_release(void);

/**
 * Initialize lexer with context
 * @params type    - Type of lexer (ATOM_LEXER_STREAM, ATOM_LEXER_STRING)
 * @params context - const char* or FILE*
 */
__atomextern int atom_lexer_init(atom_lexer_t*, int type, void* context);

__atomextern atom_node_t* atom_create(atom_type_t type, atom_text_t name);
__atomextern void         atom_delete(atom_node_t* node);

__atomextern void         atom_addchild(atom_node_t* node,
					 atom_node_t* child);

__atomextern atom_node_t* atom_parse(atom_lexer_t* lexer);

__atomextern int atom_save_file(atom_node_t* node, FILE* file);
__atomextern int atom_save_text(atom_lexer_t* lexer, atom_node_t* node,
				 char* buffer, size_t size);

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
__atomextern size_t       atom_textcpy(atom_lexer_t* lexer,
				       atom_text_t text,
				       char* buffer);
__atomextern size_t       atom_textcmp(atom_lexer_t* lexer,
				       atom_text_t text,
				       const char* string);

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

#endif /* __ATOM_H__ */


