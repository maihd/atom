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

static char* atomGetline(char* line, size_t size)
{
  assert(line != NULL);
  char c = fgetc(stdin);
  while (size-- && c != '\n' && c != '\r') {
    *line++ = c;
    c = fgetc(stdin);
  }
  *line = 0;
  return line;
}
