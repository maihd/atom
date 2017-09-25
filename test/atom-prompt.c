/**
 * Atom - file data format with s-expression
 *
 * @author: MaiHD
 * @license: Free to use
 * @copyright: MaiHD @ ${HOME}, 2017
 */

#include "atom-test.c"

int main(int argc, char* argv[])
{
  printf("Atom prompt v1.0 - MaiHD\n"); 
  atom_char_t line[1024];
  while (true) {
    printf(">> ");
    atomGetline(line, sizeof(line) - 1);
    if (strcmp(line, ".quit") == 0) {
      return 0;
    }
    atom_node_t* node = atomLoadText(line);
    if (node) {
      atomPrint(node);
      atomDelete(node);
    }
  }
  return 0;
}
