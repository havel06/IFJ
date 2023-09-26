#include "parser.h"

#include <assert.h>

#include "ast.h"
#include "lexer.h"

#define GET_TOKEN(tokenVar)                 \
	do {                                    \
		if (getNextToken(&tokenVar) != 0) { \
			return PARSE_LEXER_ERROR;       \
		}                                   \
	} while (0)

#define GET_TOKEN_ASSUME_TYPE(tokenVar, assumedType) \
	do {                                             \
		GET_TOKEN(tokenVar);                         \
		if (tokenVar.type != assumedType) {          \
			return PARSE_ERROR;                      \
		}                                            \
	} while (0)

#define CONSUME_TOKEN()               \
	do {                              \
		token consumedToken;          \
		GET_TOKEN(consumedToken);     \
		tokenDestroy(&consumedToken); \
	} while (0)

#define CONSUME_TOKEN_ASSUME_TYPE(assumedType)             \
	do {                                                   \
		token consumedToken;                               \
		GET_TOKEN_ASSUME_TYPE(consumedToken, assumedType); \
		tokenDestroy(&consumedToken);                      \
	} while (0)

#define TRY_PARSE(function)    \
	do {                       \
		int retval = function; \
		if (retval != 0) {     \
			return retval;     \
		}                      \
	} while (0)

astDataType keywordToDataType(tokenType type) {
	switch (type) {
		case TOKEN_KEYWORD_INT:
			return AST_TYPE_INT;
		case TOKEN_KEYWORD_DOUBLE:
			return AST_TYPE_DOUBLE;
		case TOKEN_KEYWORD_STRING:
			return AST_TYPE_STRING;
		default:
			assert(false);	// TODO - propagate error
	}
}

parseResult parseExpression(astExpression* expression) {
	// TODO
	(void)expression;
	return PARSE_OK;
}

parseResult parseVarDef(astStatement* statement, bool immutable) {
	statement->type = AST_STATEMENT_VAR_DEF;

	token variableNameToken;
	GET_TOKEN_ASSUME_TYPE(variableNameToken, TOKEN_IDENTIFIER);

	// TODO - omit variable type
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_COLON);

	token variableTypeToken;
	GET_TOKEN(variableTypeToken);

	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_ASSIGN);
	astExpression initExpression;
	TRY_PARSE(parseExpression(&initExpression));

	if (astVarDefCreate(&statement->variableDef, variableNameToken.content, keywordToDataType(variableTypeToken.type),
						initExpression, immutable) == 0) {
		return PARSE_OK;
	} else {
		return PARSE_INTERNAL_ERROR;
	}
}

parseResult parseStatement(astStatement* statement, const token* firstToken) {
	assert(statement);
	assert(firstToken);

	switch (firstToken->type) {
		case TOKEN_KEYWORD_VAR:
			TRY_PARSE(parseVarDef(statement, false));
			break;
		case TOKEN_KEYWORD_LET:
			TRY_PARSE(parseVarDef(statement, true));
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

		if (astProgramAdd(program, topStatement) != 0) {
			return PARSE_INTERNAL_ERROR;
		}

		tokenDestroy(&nextToken);
	}

	return PARSE_OK;
}
