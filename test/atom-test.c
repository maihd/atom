/**
 * Atom - file data format with s-expression
 *
 * @author: MaiHD
 * @license: Free to use
 * @copyright: MaiHD @ ${HOME}, 2017
 */


#include "../src/atom.h"
#include <assert.h>
#include <string.h>

atom_ctext_t types[] = {
  "ATOM_NONE",
  "ATOM_LIST",
  "ATOM_LONG",
  "ATOM_REAL",
  "ATOM_TEXT",
  "ATOM_NAME",
};

static atom_text_t atomGetline(atom_text_t line, size_t size)
{
  assert(line != NULL);
  atom_char_t c = fgetc(stdin);
  while (size-- && c != '\n' && c != '\r') {
    *line++ = c;
    c = fgetc(stdin);
  }
  *line = 0;
  return line;
}

static void atomPrint(atom_node_t* node)
{
  static int stack = 0;
  if (node) {
    for (int i = 0; i < stack; i++) {
      fputc(' ', stdout);
    }
    printf("%s - %s", types[node->type], node->name); 
    switch (node->type) {
    case ATOM_LIST:
      stack++;
      atom_node_t* children = node->children;
      if (children == NULL) {
	printf(" - (null)");
      } else {
	fputc('\n', stdout);
	while (children) {
	  atomPrint(children);
	  children = children->next;
	}
      }
      stack--;
      break;
      
    case ATOM_LONG:
      printf(" - %lld", node->value.asLong);
      break;
      
    case ATOM_REAL:
      printf(" - %lf", node->value.asReal);
      break;
      
    case ATOM_TEXT:
      printf(" - %s", node->value.asText);
      break;

    default:
      break;
    }
    fputc('\n', stdout);
  }
}
