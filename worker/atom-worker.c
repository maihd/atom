/**
 * Atom worker - command line interface to work with atom file
 *
 * @author: MaiHD
 * @license: Free to use
 * @copyright: MaiHD @ ${HOME}, 2017
 */

#define JSMN_STRICT

#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "../src/atom.h"
#include "jsmn/jsmn.h"

static char* atomGetLine(char* line, size_t size)
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

/**
 * Parse json then convert to atom node
 */
static atom_node_t* atomFromJson(const char* line, jsmntok_t* tokens,int* size);
static bool         atomJsonToAtom(const char* json, const char* atom);

int main(int argc, char* argv[])
{
  printf("Atom worker v1.0 - MaiHD\n");

  if (argc > 1) {
    if (argc < 3) {
      printf("Usage: %s <json-file> <atom-file>", argv[0]);
      return 1;
    }
    return !atomJsonToAtom(argv[1], argv[2]);
  }
  
  char line[1024];
  while (true) {
    printf(">> ");
    atomGetLine(line, sizeof(line) - 1);

    jsmn_parser parser;
    jsmntok_t token[128];
    jsmn_init(&parser);
    int r = jsmn_parse(&parser, line, sizeof(line) - 1, token, 128);
    if (r < 0) {
      printf("Failed to parse json\n");
      continue;
    }

    if (r < 1 || token[0].type != JSMN_OBJECT) {
      printf("Object expected\n");
      continue;
    }
    atom_lexer_t lexer;
    if (atomLexerInit(&lexer, ATOM_LEXER_STREAM, line)) {
      atom_node_t* node = atomFromJson(line, token, NULL);
      if (node) {
	atomSaveText(&lexer, node, line, sizeof(line) - 1);
	printf("%s\n", line);
	atomDelete(node);
      }
    }
  }
  
  return 0;
}


atom_node_t* atomFromJson(const char* context, jsmntok_t* tokens, int* size)
{
  atom_node_t* node = NULL;
  switch (tokens->type) {
  case JSMN_OBJECT:
    node = atomNewList(ATOM_TEXT_NULL);
    int count    = tokens->size;
    int nodeSize = 1;
    for (int i = 0; i < count; i++) {
      jsmntok_t* key   = tokens + nodeSize++;
      jsmntok_t* value = tokens + nodeSize++;
      
      /* Get name
       */
      atom_text_t name = { key->start, key->end - 1 };

      /* Get value
       */
      atom_node_t* child = NULL; 
      if (value->type == JSMN_ARRAY || value->type == JSMN_OBJECT) {
	int childSize;
	child = atomFromJson(context, value, &childSize);
	atomSetName(child, name);
	nodeSize += childSize - 1;
      } else {
        char  text[1024];
        char* tptr = text;
	for (int c = value->start; c < value->end; c++) {
	  *tptr++ = context[c];
	}
	*tptr = 0;
	atom_data_t data;
	if (strcmp(text, "false") == 0) {
	  child = atomNewLong(name, 0);
	} else if (strcmp(text, "true") == 0) {
	  child = atomNewLong(name, 1);
	} else if (tokens->type == JSMN_STRING) {
	  child = atomNewText(name, (atom_text_t){ value->start, value->end - 1 });
	} else if (atomToLong(text, &data)) {
	  child = atomNewLong(name, data.asLong);
	} else if (atomToReal(text, &data)) {
	  child = atomNewReal(name, data.asReal);
	} else {
	  child = atomNewText(name, (atom_text_t){ value->start, value->end - 1 });
	}
      }
      atomAddChild(node, child);
    }
    if (size) *size = nodeSize;
    break;
	
  case JSMN_ARRAY: {
    node = atomNewList(ATOM_TEXT_NULL);
    int count    = tokens->size;
    int nodeSize = 1;
    for (int i = 0; i < count; i++) {
      int childSize;
      jsmntok_t* value   = tokens + nodeSize;
      atom_node_t* child = atomFromJson(context, value, &childSize);
      atomAddChild(node, child);
      nodeSize += childSize;
    }
    if (size) *size = nodeSize; 
  } break;

  case JSMN_STRING:
  case JSMN_PRIMITIVE: {
    char  text[1024];
    char* tptr = text;
    for (int c = tokens->start; c < tokens->end; c++) {
      *tptr++ = context[c];
    }
    *tptr = 0;
    atom_data_t data;
    if (strcmp(text, "false") == 0) {
      node = atomNewLong(ATOM_TEXT_NULL, 0);
    } else if (strcmp(text, "true") == 0) {
      node = atomNewLong(ATOM_TEXT_NULL, 1);
    } else if (tokens->type == JSMN_STRING) {
      data.asText = (atom_text_t){ tokens->start, tokens->end - 1 };
      node = atomNewText(ATOM_TEXT_NULL, data.asText);
    } else if (atomToLong(text, &data)) {
      node = atomNewLong(ATOM_TEXT_NULL, data.asLong);
    } else if (atomToReal(text, &data)) {
      node = atomNewReal(ATOM_TEXT_NULL, data.asReal);
    } else {
      data.asText = (atom_text_t){ tokens->start, tokens->end - 1 };
      node = atomNewText(ATOM_TEXT_NULL, data.asText);
    }
    if (size) *size = 1;
  } break;
    
  default:
    break;
  }
  return node;
}


/* @function: 
 */
bool atomJsonToAtom(const char* json, const char* atom)
{
  /* Open given file
   */
  FILE* file = fopen(json, "r");
  if (!file) {
    fprintf(stderr, "Json not found! path: %s\n", json);
    return false;
  }

  /* Read the content from file
   */
  size_t fileSize = atomGetFileSize(file);
  char* buffer = malloc(fileSize);  /* Super large buffer */

  printf("File size: %zu\n", fileSize);
  fread(buffer, fileSize, 1, file);
  buffer[fileSize - 1] = 0;
  fclose(file); file = NULL;

  /* Parse json
   */
  jsmn_parser parser;
  jsmntok_t* tokens = malloc(sizeof(jsmntok_t) * fileSize);
  jsmn_init(&parser);
  int r = jsmn_parse(&parser, buffer, fileSize, tokens, fileSize);
  if (r < 0) {
    printf("Failed to parse json\n");
    return false;
  }
  if (r < 1 || tokens->type != JSMN_OBJECT) {
    printf("Object expected in the root of json\n");
    return false;
  }

  /* Convert json to atom
   */
  atom_node_t* node = atomFromJson(buffer, tokens, NULL);
  if (!node) {
    fprintf(stderr, "Failed to convert json to atom!");
    return false;
  }

  /* Write to file
   */
  file = fopen(atom, "w+");
  if (!file) {
    fprintf(stderr, "Open atom file for writing failed! path: %s\n", atom);
    return false;
  }
  
  bool result = true;
  atom_lexer_t lexer;
  if (atomLexerInit(&lexer, ATOM_LEXER_STRING, buffer)) {
    char* atomBuffer = malloc(fileSize * 10);
    if (!atomSaveText(&lexer, node, atomBuffer, fileSize)) {
      printf("Failed to convert json to atom!"); 
      atomDelete(node);
      return false;
    }
    
    if (fwrite(atomBuffer, strlen(atomBuffer), 1, file) != 1) {
      fprintf(stderr, "Write content to atom file failed!");
      result = false;
    }
  
    atomDelete(node);
    printf("Json to atom converted!\n");
    printf("Atom: %s\n", atomBuffer);
  } else {
    fprintf(stderr, "Failed to initialize lexer!\n");
    return false;
  }
  fclose(file);

 cleanup:
  free(buffer);
  free(tokens);
  
  printf("Atom file is written!\n");
  return result;
}
