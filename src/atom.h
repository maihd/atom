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

#ifdef ATOM32
typedef int32_t atom_long_t;
typedef float   atom_real_t;
#else
typedef int64_t atom_long_t;
typedef double  atom_real_t;
#endif
typedef char    atom_char_t;
typedef char*   atom_text_t;
typedef const char* atom_ctext_t;

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
} atom_value_t;

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
  atom_value_t value;
  atom_node_t* prev;
  atom_node_t* next;
  atom_node_t* parent;
  atom_node_t* children;
  atom_node_t* lastChild;
  //size_t       refCount;

  /* No padding needed
   */
};

/**
 * Release usage memory
 */
__atomextern void         atomRelease(void);

__atomextern atom_node_t* atomCreate(atom_type_t type, atom_ctext_t name);
__atomextern void         atomDelete(atom_node_t* node);

__atomextern atom_node_t* atomLoadFile(FILE*        file);
__atomextern atom_node_t* atomLoadText(atom_ctext_t text);

__atomextern bool atomSaveFile(atom_node_t* node, FILE* file);
__atomextern bool atomSaveText(atom_node_t* node, atom_text_t buf, size_t size);

__atominline atom_type_t  atomGetType(atom_node_t* node);
//__atominline size_t       atomGetRefCount(atom_node_t* node);
__atominline atom_node_t* atomGetParent(atom_node_t* node);
__atominline atom_node_t* atomGetChildren(atom_node_t* node);
__atominline atom_node_t* atomGetLastChild(atom_node_t* node);
__atominline atom_node_t* atomGetNextSibling(atom_node_t* node);
__atominline atom_node_t* atomGetPrevSibling(atom_node_t* node);

__atominline bool         atomIsList(atom_node_t* node);
__atominline bool         atomIsLong(atom_node_t* node);
__atominline bool         atomIsReal(atom_node_t* node);
__atominline bool         atomIsText(atom_node_t* node);

__atominline atom_node_t* atomNewList(atom_ctext_t name);
__atominline atom_node_t* atomNewLong(atom_ctext_t name, atom_long_t value);
__atominline atom_node_t* atomNewReal(atom_ctext_t name, atom_real_t value);
__atominline atom_node_t* atomNewText(atom_ctext_t name, atom_ctext_t value);

__atominline atom_ctext_t atomGetName(atom_node_t* node);
__atominline atom_long_t  atomGetLong(atom_node_t* node);
__atominline atom_real_t  atomGetReal(atom_node_t* node);
__atominline atom_ctext_t atomGetText(atom_node_t* node);

__atomextern bool         atomSetName(atom_node_t* node, atom_ctext_t name);
__atomextern bool         atomSetLong(atom_node_t* node, atom_long_t value);
__atomextern bool         atomSetReal(atom_node_t* node, atom_real_t value);
__atomextern bool         atomSetText(atom_node_t* node, atom_ctext_t value);

/******
 * Inline functions definition
 */
__atominline atom_type_t  atomGetType(atom_node_t* node)
{
  return node->type;
}


__atominline atom_node_t* atomGetParent(atom_node_t* node)
{
  return node->parent;
}


__atominline atom_node_t* atomGetChildren(atom_node_t* node)
{
  return node->children;
}


__atominline atom_node_t* atomGetLastChild(atom_node_t* node)
{
  return node->lastChild;
}


__atominline atom_node_t* atomGetNextSibling(atom_node_t* node)
{
  return node->next;
}


__atominline atom_node_t* atomGetPrevSibling(atom_node_t* node)
{
  return node->prev;
}


__atominline bool         atomIsList(atom_node_t* node)
{
  return node->type == ATOM_LIST;
}


__atominline bool         atomIsLong(atom_node_t* node)
{
  return node->type == ATOM_LONG;
}


__atominline bool         atomIsReal(atom_node_t* node)
{
  return node->type == ATOM_REAL;
}


__atominline bool         atomIsText(atom_node_t* node)
{
  return node->type == ATOM_TEXT;
}



__atominline atom_ctext_t atomGetName(atom_node_t* node)
{
  return node->name;
}


__atominline atom_long_t  atomGetLong(atom_node_t* node)
{
  return node->value.asLong;
}


__atominline atom_real_t  atomGetReal(atom_node_t* node)
{
  return node->value.asReal;
}


__atominline atom_ctext_t atomGetText(atom_node_t* node)
{
  return node->value.asText;
}


__atominline atom_node_t* atomNewList(atom_ctext_t name)
{
  atom_node_t* node = atomCreate(ATOM_LIST, name);
  node->value.asRoot = false;
  return node;
}


__atominline atom_node_t* atomNewLong(atom_ctext_t name, atom_long_t value)
{
  atom_node_t* node = atomCreate(ATOM_LONG, name);
  node->value.asLong = value;
  return node;
}


__atominline atom_node_t* atomNewReal(atom_ctext_t name, atom_real_t value)
{
  atom_node_t* node = atomCreate(ATOM_REAL, name);
  node->value.asReal = value;
  return node;
}


__atominline atom_node_t* atomNewText(atom_ctext_t name, atom_ctext_t value)
{
  atom_node_t* node = atomCreate(ATOM_TEXT, name);
  atomSetText(node, value);
  return node;
}

/*




__atominline bool         atomSetName(atom_node_t* node, const char* name)
{
  node->name = name;
  return true;
}

__atominline bool         atomSetLong(atom_node_t* node, long value)
{
  if (node->type != ATOM_LONG) {
    return false;
  }

  node->value.asLong = value;
  return true;
}


__atominline bool         atomSetDouble(atom_node_t* node, double value)
{
  if (node->type != ATOM_DOUBLE) {
    return false;
  }

  node->value.asDouble = value;
  return true;
}


__atominline bool         atomSetString(atom_node_t* node, const char* value)
{
  if (node->type != ATOM_STRING) {
    return false;
  }

  node->value.asString = value;
  return true;
}*/

#endif /* __ATOM_H__ */


