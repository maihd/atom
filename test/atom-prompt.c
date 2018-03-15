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
    while (ATOM_TRUE)
    {
	printf(">> ");
	atom_getline(line, sizeof(line) - 1);
	if (strcmp(line, ".quit") == 0)
	{
	    return 0;
	}
	
	if (atom_lexer_init(&lexer, ATOM_LEXER_STRING, line))
	{
	    atom_node_t* node = atom_parse(&lexer);
	    if (node)
	    {
		atom_print(&lexer, node);
		atom_delete(node);
	    }
	}
    }
    return 0;
}
