/**
 * Atom - file data format with s-expression
 *
 * @author: MaiHD
 * @license: Free to use
 * @copyright: MaiHD @ ${HOME}, 2017
 */

#ifndef __ATOM_H__
#define __ATOM_H__

#include <stdio.h>
#include <stdlib.h>

/******
 * Metadata constants
 */
#define ATOM_LIBNAME "libatom"
#define ATOM_VERSION 100000000

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
#if !defined(__cplusplus) && !defined(bool)
typedef int bool;
#endif
#if !defined(__cplusplus) && !defined(true)
enum { true = 1 };  /* Prevent undef (maybe wrong :D) */
#endif
#if !defined(__cplusplus) && !defined(false)
enum { false = 0 }; /* Prevent undef (maybe wrong :D) */
#endif
typedef int64_t atom_long_t; /* Integer number  */
typedef double  atom_real_t; /* Real    number  */
typedef struct               /* A text in lexer */
{
  uint32_t head;
  uint32_t tail;
} atom_text_t;


/**
 * Atom value type
 * @note: commented-line is the feature in decision
 */
typedef enum {
  ATOM_NONE = 0, /* Error data type */
  ATOM_LIST,
  ATOM_LONG,   
  ATOM_REAL,
  ATOM_TEXT,
  ATOM_NAME,      /* Only in the first of list, after parse will be the name */
} atom_type_t;


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
 * Atom immediately present value
 * @size: fixed-size, runtime dependent (4bytes on 32bits, 8bytes on 64bits)
 * @note: commented-line is the feature in decision
 */
typedef union
{
  atom_long_t asRoot; /* Available when is list */
  atom_long_t asLong;
  atom_real_t asReal;
  atom_text_t asText;
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
  atom_node_t* lastChild;

  /* No padding needed
   */
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
    const char* string;
    FILE*       stream;
  } buffer;

  int    errcode;       /* Error code        */
  int    errcursor;     /* Position at error */
  
  /* No padding needed
   */
} atom_lexer_t;

/**
 * Global constants
 */
extern const atom_text_t ATOM_TEXT_NULL;


/**
 * Release all usage memories by atom module
 */
__atomextern void         atomRelease(void);

/**
 * Initialize lexer with context
 * @params type    - Type of lexer (ATOM_LEXER_STREAM, ATOM_LEXER_STRING)
 * @params context - const char* or FILE*
 */
__atomextern bool         atomLexerInit(atom_lexer_t*, int type, void* context);

__atomextern atom_node_t* atomCreate(atom_type_t type, atom_text_t name);
__atomextern void         atomDelete(atom_node_t* node);

__atomextern void         atomAddChild(atom_node_t* node, atom_node_t* child);

__atomextern atom_node_t* atomParse(atom_lexer_t* lexer);

__atomextern bool atomSaveFile(atom_node_t* node, FILE* file);
__atomextern bool atomSaveText(atom_lexer_t* lexer, atom_node_t* node,
			       char* buffer, size_t size);

__atominline atom_type_t  atomGetType(atom_node_t* node);
__atominline atom_node_t* atomGetParent(atom_node_t* node);
__atominline atom_node_t* atomGetChildren(atom_node_t* node);
__atominline atom_node_t* atomGetLastChild(atom_node_t* node);
__atominline atom_node_t* atomGetNextSibling(atom_node_t* node);
__atominline atom_node_t* atomGetPrevSibling(atom_node_t* node);

__atominline bool         atomIsRoot(atom_node_t* node);
__atominline bool         atomIsList(atom_node_t* node);
__atominline bool         atomIsLong(atom_node_t* node);
__atominline bool         atomIsReal(atom_node_t* node);
__atominline bool         atomIsText(atom_node_t* node);

__atominline atom_node_t* atomNewList(atom_text_t name);
__atominline atom_node_t* atomNewLong(atom_text_t name, atom_long_t value);
__atominline atom_node_t* atomNewReal(atom_text_t name, atom_real_t value);
__atominline atom_node_t* atomNewText(atom_text_t name, atom_text_t value);

__atominline atom_text_t  atomGetName(atom_node_t* node);
__atominline atom_long_t  atomGetLong(atom_node_t* node);
__atominline atom_real_t  atomGetReal(atom_node_t* node);
__atominline atom_text_t  atomGetText(atom_node_t* node);

__atomextern bool         atomSetName(atom_node_t* node, atom_text_t name);
__atomextern bool         atomSetLong(atom_node_t* node, atom_long_t value);
__atomextern bool         atomSetReal(atom_node_t* node, atom_real_t value);
__atomextern bool         atomSetText(atom_node_t* node, atom_text_t value);

/******
 * Utilities
 */

__atomextern size_t atomGetFileSize(FILE* file);
__atomextern void   atomPrint(atom_lexer_t* lexer, atom_node_t* node);
__atomextern bool   atomToLong(const char* text, atom_data_t* value);
__atomextern bool   atomToReal(const char* text, atom_data_t* value);

/******
 * Inline functions definition
 */


/* @function: atomGetType
 */
__atominline atom_type_t  atomGetType(atom_node_t* node)
{
  return node->type;
}


/* @function: atomGetParent
 */
__atominline atom_node_t* atomGetParent(atom_node_t* node)
{
  return node->parent;
}


/* @function: atomGetChildren
 */
__atominline atom_node_t* atomGetChildren(atom_node_t* node)
{
  return node->children;
}


/* @function: atomGetLastChild
 */
__atominline atom_node_t* atomGetLastChild(atom_node_t* node)
{
  return node->lastChild;
}


/* @function: atomGetNextSibling
 */
__atominline atom_node_t* atomGetNextSibling(atom_node_t* node)
{
  return node->next;
}


/* @function: atomGetPrevSibling
 */
__atominline atom_node_t* atomGetPrevSibling(atom_node_t* node)
{
  return node->prev;
}


/* @function: atomIsRoot
 */
__atominline bool         atomIsRoot(atom_node_t* node)
{
  return atomIsList(node) && node->data.asRoot;
}


/* @function: atomIsList
 */ 
__atominline bool         atomIsList(atom_node_t* node)
{
  return node->type == ATOM_LIST;
}


/* @function: atomIsLong
 */
__atominline bool         atomIsLong(atom_node_t* node)
{
  return node->type == ATOM_LONG;
}


/* @function: atomIsReal
 */
__atominline bool         atomIsReal(atom_node_t* node)
{
  return node->type == ATOM_REAL;
}


/* @function: atomIsText
 */
__atominline bool         atomIsText(atom_node_t* node)
{
  return node->type == ATOM_TEXT;
}


/* @function: atomGetName
 */
__atominline atom_text_t  atomGetName(atom_node_t* node)
{
  return node->name;
}


/* @function: atomGetLong
 */
__atominline atom_long_t  atomGetLong(atom_node_t* node)
{
  return node->data.asLong;
}


/* @function: atomGetReal
 */
__atominline atom_real_t  atomGetReal(atom_node_t* node)
{
  return node->data.asReal;
}


/* @function: atomGetText
 */
__atominline atom_text_t  atomGetText(atom_node_t* node)
{
  return node->data.asText;
}


/* @function: atomNewList
 */
__atominline atom_node_t* atomNewList(atom_text_t name)
{
  atom_node_t* node = atomCreate(ATOM_LIST, name);
  node->data.asRoot = false;
  return node;
}


/* @function: atomNewLong
 */
__atominline atom_node_t* atomNewLong(atom_text_t name, atom_long_t value)
{
  atom_node_t* node = atomCreate(ATOM_LONG, name);
  node->data.asLong = value;
  return node;
}


/* @function: atomNewReal
 */
__atominline atom_node_t* atomNewReal(atom_text_t name, atom_real_t value)
{
  atom_node_t* node = atomCreate(ATOM_REAL, name);
  node->data.asReal = value;
  return node;
}


/* @function: atomNewText
 */
__atominline atom_node_t* atomNewText(atom_text_t name, atom_text_t value)
{
  atom_node_t* node = atomCreate(ATOM_TEXT, name);
  node->data.asText = value;
  return node;
}

#endif /* __ATOM_H__ */


