#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

typedef enum { PARSE_OK, PARSE_LEXER_ERROR, PARSE_ERROR, PARSE_INTERNAL_ERROR } parseResult;

parseResult parseProgram(astProgram*);

#endif
