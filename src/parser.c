#include "parser.h"

#include <assert.h>
#include <stdio.h>

#include "ast.h"
#include "lexer.h"
#include "printToken.h"

// TODO - destroy tokens when any macro returns error

#define GET_TOKEN(tokenVar, onError)         \
	do {                                     \
		switch (getNextToken(&tokenVar)) {   \
			case LEXER_ERROR:                \
				onError;                     \
				return PARSE_LEXER_ERROR;    \
			case LEXER_INTERNAL_ERROR:       \
				onError;                     \
				return PARSE_INTERNAL_ERROR; \
			case LEXER_OK:                   \
				break;                       \
		}                                    \
	} while (0)

#define GET_TOKEN_ASSUME_TYPE(tokenVar, assumedType, onError) \
	do {                                                      \
		GET_TOKEN(tokenVar, onError);                         \
		if (tokenVar.type != assumedType) {                   \
			onError;                                          \
			return PARSE_ERROR;                               \
		}                                                     \
	} while (0)

#define CONSUME_TOKEN(onError)             \
	do {                                   \
		token consumedToken;               \
		GET_TOKEN(consumedToken, onError); \
		tokenDestroy(&consumedToken);      \
	} while (0)

#define CONSUME_TOKEN_ASSUME_TYPE(assumedType, onError)             \
	do {                                                            \
		token consumedToken;                                        \
		GET_TOKEN_ASSUME_TYPE(consumedToken, assumedType, onError); \
		tokenDestroy(&consumedToken);                               \
	} while (0)

#define TRY_PARSE(function, onError) \
	do {                             \
		int retval = function;       \
		if (retval != 0) {           \
			onError;                 \
			return retval;           \
		}                            \
	} while (0)

astBasicDataType keywordToDataType(tokenType type) {
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
	GET_TOKEN_ASSUME_TYPE(variableNameToken, TOKEN_IDENTIFIER, {});

	// TODO - omit variable type
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_COLON, { tokenDestroy(&variableNameToken); });

	token variableTypeToken;
	GET_TOKEN(variableTypeToken, { tokenDestroy(&variableNameToken); });

	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_ASSIGN, {
		tokenDestroy(&variableNameToken);
		tokenDestroy(&variableTypeToken);
	});

	astExpression initExpression;
	TRY_PARSE(parseExpression(&initExpression), {
		tokenDestroy(&variableNameToken);
		tokenDestroy(&variableTypeToken);
	});

	astDataType dataType;
	dataType.type = keywordToDataType(variableTypeToken.type);
	dataType.nullable = false;
	// TODO - parse nullable types

	parseResult returnValue = PARSE_OK;
	if (astVarDefCreate(&statement->variableDef, variableNameToken.content, dataType, initExpression, immutable) != 0) {
		returnValue = PARSE_INTERNAL_ERROR;
	}

	return returnValue;
}

parseResult parseStatement(astStatement* statement, const token* firstToken) {
	assert(statement);
	assert(firstToken);

	switch (firstToken->type) {
		case TOKEN_KEYWORD_VAR:
			TRY_PARSE(parseVarDef(statement, false), {});
			break;
		case TOKEN_KEYWORD_LET:
			TRY_PARSE(parseVarDef(statement, true), {});
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
			fprintf(stderr, "Unexpected token ");
			printToken(firstToken, stderr);
			fprintf(stderr, "on start of statement.\n");
			return PARSE_ERROR;
	}

	return PARSE_OK;
}

parseResult parseProgram(astProgram* program) {
	token nextToken;

	while (1) {
		GET_TOKEN(nextToken, {});
		astTopLevelStatement topStatement;

		if (nextToken.type == TOKEN_EOF) {
			break;
		} else if (nextToken.type == TOKEN_KEYWORD_FUNC) {
			topStatement.type = AST_TOP_FUNCTION;
			// TODO - parse function definition
		} else {
			topStatement.type = AST_TOP_STATEMENT;
			TRY_PARSE(parseStatement(&topStatement.statement, &nextToken), { tokenDestroy(&nextToken); });
		}

		tokenDestroy(&nextToken);

		if (astProgramAdd(program, topStatement) != 0) {
			return PARSE_INTERNAL_ERROR;
		}
	}

	return PARSE_OK;
}
