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
    printf("Atom viewer v1.0 - MaiHD\n");

    /* Viewer require a file name
     */
    if (argc < 2)
    {
	fprintf(stderr, "usage: %s <name>\n", argv[0]);
	return 1;
    }

    /* Load file
     */
    const char* filename = argv[1];
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
	fprintf(stderr, "File not found! path: %s\n", filename);
	return 1;
    }
    
    atom_node_t* node = NULL;
    atom_lexer_t lexer;
    if (atom_lexer_init(&lexer, ATOM_LEXER_STREAM, file) == ATOM_ERROR_NONE)
    {
	node = atom_parse(&lexer);
	if (node)
	{
	    atom_print(&lexer, node);
	    atom_delete(node);
	}
	else
	{
	    fprintf(stderr, "Parsing error!\n");
	}
    }
    else
    {
	fprintf(stderr, "Initialize lexer failed!\n");
    }
    fclose(file);
  
    return node == NULL;
}
