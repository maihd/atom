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
  char line[1024];
  atom_lexer_t lexer;
  while (true) {
    printf(">> ");
    atomGetline(line, sizeof(line) - 1);
    if (strcmp(line, ".quit") == 0) {
      return 0;
    }
    if (atomLexerInit(&lexer, ATOM_LEXER_STRING, line)) {
      atom_node_t* node = atomParse(&lexer);
      if (node) {
	atomPrint(&lexer, node);
	atomDelete(node);
      }
    }
  }
  return 0;
}
