#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void parseIntLiteral(const token* tok, astExpression* expr) {
	expr->type = AST_EXPR_TERM;
	expr->term.type = AST_TERM_INT;
	expr->term.integer.value = atoi(tok->content);
	// TODO - parse exponent notation correctly
}

void parseDecimalLiteral(const token* tok, astExpression* expr) {
	expr->type = AST_EXPR_TERM;
	expr->term.type = AST_TERM_DECIMAL;
	expr->term.decimal.value = atof(tok->content);
}

parseResult parseStringLiteral(const token* tok, astExpression* expr) {
	expr->type = AST_EXPR_TERM;
	expr->term.type = AST_TERM_STRING;
	expr->term.string.content = malloc(strlen(tok->content) * sizeof(char) + 1);
	if (!expr->term.string.content) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(expr->term.string.content, tok->content);

	return PARSE_OK;
}

parseResult parseIdentifier(const token* tok, astExpression* expr) {
	expr->type = AST_EXPR_TERM;
	expr->term.type = AST_TERM_ID;
	expr->term.identificator.name = malloc(strlen(tok->content) + 1);
	if (!expr->term.string.content) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(expr->term.identificator.name, tok->content);

	return PARSE_OK;
}

parseResult parseExpression(astExpression* expression, token* firstToken); // fwd

parseResult parsePrimaryExpression(astExpression* expression, token* firstToken) {
	switch (firstToken->type) {
		case TOKEN_INT_LITERAL:
			parseIntLiteral(firstToken, expression);
			break;
		case TOKEN_DEC_LITERAL:
			parseDecimalLiteral(firstToken, expression);
			break;
		case TOKEN_STR_LITERAL:
			TRY_PARSE(parseStringLiteral(firstToken, expression), {});
			break;
		case TOKEN_BRACKET_ROUND_LEFT: {
			token nextToken;
			GET_TOKEN(nextToken, {});
			TRY_PARSE(parseExpression(expression, &nextToken), {
				tokenDestroy(&nextToken);
			});
			tokenDestroy(&nextToken);
			CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});
			break;
		}
		case TOKEN_IDENTIFIER:
			TRY_PARSE(parseIdentifier(firstToken, expression), {});
			break;
		// TODO - unwrap expression
		default:
			fprintf(stderr, "Unexpected token ");
			printToken(firstToken, stderr);
			fprintf(stderr, "on start of expression.\n");
			return PARSE_ERROR;
	}

	return PARSE_OK;
}

bool isBinaryOperator(tokenType type) {
	switch (type) {
		case TOKEN_NEQ:
		case TOKEN_LESS:
		case TOKEN_GREATER:
		case TOKEN_LESS_EQ:
		case TOKEN_GREATER_EQ:
		case TOKEN_MUL:
		case TOKEN_DIV:
		case TOKEN_PLUS:
		case TOKEN_MINUS:
		case TOKEN_COALESCE:
			return true;
		default:
			return false;
	}
}

astBinaryOperator parseBinaryOperator(tokenType type) {
	switch (type) {
		case TOKEN_NEQ:
			return AST_BINARY_NEQ;
		case TOKEN_LESS:
			return AST_BINARY_LESS;
		case TOKEN_GREATER:
			return AST_BINARY_GREATER;
		case TOKEN_LESS_EQ:
			return AST_BINARY_LESS_EQ;
		case TOKEN_GREATER_EQ:
			return AST_BINARY_GREATER_EQ;
		case TOKEN_MUL:
			return AST_BINARY_MUL;
		case TOKEN_DIV:
			return AST_BINARY_DIV;
		case TOKEN_PLUS:
			return AST_BINARY_PLUS;
		case TOKEN_MINUS:
			return AST_BINARY_MINUS;
		case TOKEN_COALESCE:
			return AST_BINARY_NIL_COAL;
		default:
			assert(false);
	}
}

int operatorPrecedence(astBinaryOperator op) {
	switch (op) {
		case AST_BINARY_MUL:
		case AST_BINARY_DIV:
			return 3;
		case AST_BINARY_PLUS:
		case AST_BINARY_MINUS:
			return 2;
		case AST_BINARY_EQ:
		case AST_BINARY_NEQ:
		case AST_BINARY_LESS:
		case AST_BINARY_GREATER:
		case AST_BINARY_LESS_EQ:
		case AST_BINARY_GREATER_EQ:
			return 1;
		case AST_BINARY_NIL_COAL:
			return 0;
	}
	assert(false);
}

parseResult parseExpression(astExpression* expression, token* exprFirstToken) {
	astExpression subExpressions[512];
	int subExpressionsCount = 0;
	astBinaryOperator operators[512];
	int operatorCount = 0;

	// TODO - error when array overflows
	TRY_PARSE(parsePrimaryExpression(&subExpressions[subExpressionsCount++], exprFirstToken), {});

	while (1) {
		token firstToken;
		GET_TOKEN(firstToken, {});

		if (!isBinaryOperator(firstToken.type)) {
			unGetToken(&firstToken);
			break;
		}

		// TODO - error when array overflows
		operators[operatorCount++] = parseBinaryOperator(firstToken.type);
		tokenDestroy(&firstToken);

		token subExprFirstToken;
		GET_TOKEN(subExprFirstToken, {});
		printToken(&subExprFirstToken, stdout);
		TRY_PARSE(parsePrimaryExpression(&subExpressions[subExpressionsCount++], &subExprFirstToken), {
			tokenDestroy(&subExprFirstToken);
		});
		tokenDestroy(&subExprFirstToken);
	}

	// TODO - operator associativity?
	while (operatorCount > 0) {
		// apply operator precedence
		int maxPrecedence = 0;
		int maxPrecedenceIndex = 0;
		// scan operators for highest precedence
		for (int i = 0; i < operatorCount; i++) {
			int precednece = operatorPrecedence(operators[i]);
			if (precednece > maxPrecedence) {
				maxPrecedence = precednece;
				maxPrecedenceIndex = i;
			}
		}
		// join expressions
		astExpression newBinaryExpr;
		astBinaryExprCreate(&newBinaryExpr, subExpressions[maxPrecedenceIndex], subExpressions[maxPrecedenceIndex + 1],
							operators[maxPrecedenceIndex]);
		subExpressions[maxPrecedenceIndex] = newBinaryExpr;
		// rotate the rest of the expressions to the left to remove rhs
		for (int i = maxPrecedenceIndex + 1; i < subExpressionsCount - 1; i++) {
			subExpressions[i] = subExpressions[i + 1];
		}
		// rotate the rest of the operators to the left to remove used operator
		for (int i = maxPrecedenceIndex + 1; i < operatorCount - 1; i++) {
			operators[i] = operators[i + 1];
		}

		operatorCount--;
		subExpressionsCount--;
	}

	*expression = subExpressions[0];
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
	token initExprFirstToken;

	GET_TOKEN(initExprFirstToken, {
		tokenDestroy(&variableNameToken);
		tokenDestroy(&variableTypeToken);
	});

	TRY_PARSE(parseExpression(&initExpression, &initExprFirstToken), {
		tokenDestroy(&variableNameToken);
		tokenDestroy(&variableTypeToken);
		tokenDestroy(&initExprFirstToken);
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

parseResult parseFunctionCall(astStatement* statement, const char* varName, const char* funcName) {
	(void)statement;
	(void)varName;
	(void)funcName;
	return PARSE_OK;
	//TODO
}

parseResult parseVoidFunctionCall(astStatement* statement, const char* funcName) {
	(void)statement;
	(void)funcName;
	return PARSE_OK;
	// TODO
}

parseResult parseAssignment(astStatement* statement, const char* varName, token* exprFirstToken) {
	statement->type = AST_STATEMENT_ASSIGN;
	statement->assignment.variableName = malloc(strlen(varName) + 1);
	if (!statement->assignment.variableName) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(statement->assignment.variableName, varName);
	TRY_PARSE(parseExpression(&(statement->assignment.value), exprFirstToken), {});
	return PARSE_OK;
}

parseResult parseAssignmentOrFunctionCall(astStatement* statement, const char* varName) {
	token idToken;
	GET_TOKEN_ASSUME_TYPE(idToken, TOKEN_IDENTIFIER, {});

	token nextToken;
	GET_TOKEN(nextToken, {
		tokenDestroy(&idToken);
	});

	if (nextToken.type == TOKEN_BRACKET_ROUND_LEFT) {
		TRY_PARSE(parseFunctionCall(statement, varName, nextToken.content), {
			tokenDestroy(&idToken);
			tokenDestroy(&nextToken);
		});
		tokenDestroy(&nextToken);
	} else {
		unGetToken(&nextToken);
		TRY_PARSE(parseAssignment(statement, varName, &idToken), {
			tokenDestroy(&idToken);
		});
	}
	tokenDestroy(&idToken);

	return PARSE_OK;
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
		case TOKEN_IDENTIFIER: {
			token secondToken;
			GET_TOKEN(secondToken, {});
			tokenType secondTokenType = secondToken.type;
			tokenDestroy(&secondToken);
			if (secondTokenType == TOKEN_ASSIGN) {
				TRY_PARSE(parseAssignmentOrFunctionCall(statement, firstToken->content), {});
			} else {
				TRY_PARSE(parseVoidFunctionCall(statement, firstToken->content), {});
			}
			break;
		}
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
