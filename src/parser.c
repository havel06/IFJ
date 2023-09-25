#include "parser.h"

#include <assert.h>

#include "ast.h"
#include "lexer.h"

#define GET_TOKEN(tokenVar)                  \
	do {                                     \
		if (getNextToken(&nextToken) != 0) { \
			return PARSE_LEXER_ERROR;        \
		}                                    \
	} while (0)

#define TRY_PARSE(function)     \
	do {                        \
		if (function != 0) {    \
			return PARSE_ERROR; \
		}                       \
	} while (0)

int parseStatement(astStatement* statement, const token* firstToken) {
	assert(statement);
	assert(firstToken);

	switch (firstToken->type) {
		case TOKEN_KEYWORD_VAR:
			// TODO - parse variable definition
			break;
		case TOKEN_KEYWORD_IF:
			// TODO - parse conditional
			break;
		case TOKEN_KEYWORD_WHILE:
			// TODO - parse iteration
			break;
		case TOKEN_IDENTIFIER:
			// TODO - parse variable assignment or function call
			break;
		default:
			return 1;
	}

	return 0;
}

parseResult parseProgram(astProgram* program) {
	token nextToken;

	while (1) {
		GET_TOKEN(nextToken);
		astTopLevelStatement topStatement;

		if (nextToken.type == TOKEN_EOF) {
			break;
		} else if (nextToken.type == TOKEN_KEYWORD_FUNC) {
			topStatement.type = AST_TOP_FUNCTION;
			// TODO - parse function definition
		} else {
			topStatement.type = AST_TOP_STATEMENT;
			TRY_PARSE(parseStatement(&topStatement.statement, &nextToken));
		}

		astProgramAdd(program, topStatement);
		tokenDestroy(&nextToken);
	}

	return PARSE_OK;
}
