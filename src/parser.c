#include "parser.h"

#include "lexer.h"

#define GET_TOKEN(tokenVar)                  \
	do {                                     \
		if (getNextToken(&nextToken) != 0) { \
			return PARSE_LEXER_ERROR;        \
		}                                    \
	} while (0)

parseResult parseProgram(astProgram *program) {
	token nextToken;

	GET_TOKEN(nextToken);
	// TODO
	(void)program;

	tokenDestroy(&nextToken);

	return PARSE_OK;
}
