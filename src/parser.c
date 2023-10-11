#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "printToken.h"

// forward declarations
static parseResult parseStatement(astStatement* statement, const token* firstToken, bool insideFunction);
static parseResult parseExpression(astExpression* expression, const token* firstToken);

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

#define GET_TOKEN_TYPE(typeVar, onError) \
	do {                                 \
		token testedToken;               \
		GET_TOKEN(testedToken, onError); \
		typeVar = testedToken.type;      \
		tokenDestroy(&testedToken);      \
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

static astBasicDataType keywordToDataType(tokenType type) {
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

static void parseIntLiteral(const token* tok, astTerm* term) {
	term->type = AST_TERM_INT;
	term->integer.value = atoi(tok->content);
	// TODO - parse exponent notation correctly
}

static void parseDecimalLiteral(const token* tok, astTerm* term) {
	term->type = AST_TERM_DECIMAL;
	term->decimal.value = atof(tok->content);
}

static parseResult parseStringLiteral(const token* tok, astTerm* term) {
	term->type = AST_TERM_STRING;
	term->string.content = malloc(strlen(tok->content) * sizeof(char) + 1);
	if (!term->string.content) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(term->string.content, tok->content);

	return PARSE_OK;
}

static parseResult parseIdentifier(const token* tok, astIdentifier* identifier) {
	identifier->name = malloc(strlen(tok->content) + 1);
	if (!identifier->name) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(identifier->name, tok->content);
	return PARSE_OK;
}

static parseResult parseIdentifierTerm(const token* tok, astTerm* term) {
	term->type = AST_TERM_ID;
	TRY_PARSE(parseIdentifier(tok, &(term->identifier)), {});
	return PARSE_OK;
}

static parseResult parseTerm(astTerm* term, const token* firstToken) {
	switch (firstToken->type) {
		case TOKEN_KEYWORD_NIL:
			term->type = AST_TERM_NIL;
			break;
		case TOKEN_INT_LITERAL:
			parseIntLiteral(firstToken, term);
			break;
		case TOKEN_DEC_LITERAL:
			parseDecimalLiteral(firstToken, term);
			break;
		case TOKEN_STR_LITERAL:
			TRY_PARSE(parseStringLiteral(firstToken, term), {});
			break;
		case TOKEN_IDENTIFIER:
			TRY_PARSE(parseIdentifierTerm(firstToken, term), {});
			break;
		default:
			fprintf(stderr, "Unexpected token ");
			printToken(firstToken, stderr);
			fprintf(stderr, "on start of term.\n");
			return PARSE_ERROR;
	}

	return PARSE_OK;
}

static parseResult parsePrimaryExpression(astExpression* expression, const token* firstToken) {
	switch (firstToken->type) {
		case TOKEN_BRACKET_ROUND_LEFT: {
			token nextToken;
			GET_TOKEN(nextToken, {});
			TRY_PARSE(parseExpression(expression, &nextToken), { tokenDestroy(&nextToken); });
			tokenDestroy(&nextToken);
			CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});
			break;
		}
		default:
			expression->type = AST_EXPR_TERM;
			TRY_PARSE(parseTerm(&expression->term, firstToken), {});
			break;
	}

	// unwrap expression
	token nextToken;
	GET_TOKEN(nextToken, {});
	if (nextToken.type == TOKEN_UNWRAP) {
		tokenDestroy(&nextToken);
		astExpression unwrapExpression;
		unwrapExpression.type = AST_EXPR_UNWRAP;
		unwrapExpression.unwrap.innerExpr = malloc(sizeof(astExpression));
		if (!unwrapExpression.unwrap.innerExpr) {
			return PARSE_INTERNAL_ERROR;
		}
		*(unwrapExpression.unwrap.innerExpr) = *expression;
		*expression = unwrapExpression;
	} else {
		unGetToken(&nextToken);
	}

	return PARSE_OK;
}

static bool isBinaryOperator(tokenType type) {
	switch (type) {
		case TOKEN_EQ:
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

static astBinaryOperator parseBinaryOperator(tokenType type) {
	switch (type) {
		case TOKEN_EQ:
			return AST_BINARY_EQ;
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

static int operatorPrecedence(astBinaryOperator op) {
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

static parseResult parseExpression(astExpression* expression, const token* exprFirstToken) {
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
		TRY_PARSE(parsePrimaryExpression(&subExpressions[subExpressionsCount++], &subExprFirstToken),
				  { tokenDestroy(&subExprFirstToken); });
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

static parseResult parseDataType(astDataType* dataType) {
	token tok;
	GET_TOKEN(tok, {});
	dataType->type = keywordToDataType(tok.type);
	tokenDestroy(&tok);

	token optToken;
	GET_TOKEN(optToken, {});

	// parse nullable part
	if (optToken.type == TOKEN_QUESTION_MARK) {
		dataType->nullable = true;
		tokenDestroy(&optToken);
	} else {
		dataType->nullable = false;
		unGetToken(&optToken);
	}

	return PARSE_OK;
}

static parseResult parseVarDef(astStatement* statement, bool immutable) {
	statement->type = AST_STATEMENT_VAR_DEF;
	statement->variableDef.immutable = immutable;

	token variableNameToken;
	GET_TOKEN_ASSUME_TYPE(variableNameToken, TOKEN_IDENTIFIER, {});
	TRY_PARSE(parseIdentifier(&variableNameToken, &(statement->variableDef.variableName)), {});
	tokenDestroy(&variableNameToken);

	// parse variable type
	token maybeColonToken;
	GET_TOKEN(maybeColonToken, {});
	if (maybeColonToken.type == TOKEN_COLON) {
		statement->variableDef.hasExplicitType = true;
		tokenDestroy(&maybeColonToken);
		TRY_PARSE(parseDataType(&(statement->variableDef.variableType)), {});
	} else {
		statement->variableDef.hasExplicitType = false;
		unGetToken(&maybeColonToken);
	}

	// omit init value
	if (statement->variableDef.hasExplicitType) {
		token maybeAssign;
		GET_TOKEN(maybeAssign, {});
		if (maybeAssign.type != TOKEN_ASSIGN) {
			statement->variableDef.hasInitValue = false;
			unGetToken(&maybeAssign);
			return PARSE_OK;
		}
		unGetToken(&maybeAssign);
	}

	// parse init value
	statement->variableDef.hasInitValue = true;
	statement->variableDef.value.type = AST_VAR_INIT_EXPR;
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_ASSIGN, {});

	token initExprFirstToken;
	GET_TOKEN(initExprFirstToken, {});

	TRY_PARSE(parseExpression(&(statement->variableDef.value.expr), &initExprFirstToken),
			  { tokenDestroy(&initExprFirstToken); });

	tokenDestroy(&initExprFirstToken);

	return PARSE_OK;
}

static parseResult parseStatementBlock(astStatementBlock* block, bool insideFunction) {
	astStatementBlockCreate(block);
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_CURLY_LEFT, {});
	while (1) {
		token nextToken;
		GET_TOKEN(nextToken, {});

		if (nextToken.type == TOKEN_BRACKET_CURLY_RIGHT) {
			tokenDestroy(&nextToken);
			break;
		}
		astStatement statement;
		TRY_PARSE(parseStatement(&statement, &nextToken, insideFunction), { tokenDestroy(&nextToken); });

		if (astStatementBlockAdd(block, statement) != 0) {
			tokenDestroy(&nextToken);
			return PARSE_INTERNAL_ERROR;
		}

		tokenDestroy(&nextToken);
	}
	return PARSE_OK;
}

static parseResult parseFunctionCallParameter(astInputParameter* param) {
	token firstToken;
	GET_TOKEN(firstToken, {});

	// check if parameter has a name
	param->hasName = false;
	if (firstToken.type == TOKEN_IDENTIFIER) {
		token secondToken;
		GET_TOKEN(secondToken, {});

		if (secondToken.type == TOKEN_COLON) {
			param->hasName = true;
			TRY_PARSE(parseIdentifier(&firstToken, &(param->name)), { tokenDestroy(&firstToken); });
			// replace first token with value token
			tokenDestroy(&firstToken);
			GET_TOKEN(firstToken, {});
		} else {
			unGetToken(&secondToken);
		}
	}

	TRY_PARSE(parseTerm(&(param->value), &firstToken), { tokenDestroy(&firstToken); });
	tokenDestroy(&firstToken);
	return PARSE_OK;
}

static parseResult parseFunctionCallParams(astInputParameterList* params) {
	astInputParameterListCreate(params);

	token firstToken;
	GET_TOKEN(firstToken, {});
	if (firstToken.type == TOKEN_BRACKET_ROUND_RIGHT) {
		unGetToken(&firstToken);
		return PARSE_OK;
	}
	unGetToken(&firstToken);

	astInputParameter firstParam;
	TRY_PARSE(parseFunctionCallParameter(&firstParam), {});
	if (astInputParameterListAdd(params, firstParam) != 0) {
		return PARSE_INTERNAL_ERROR;
	}

	while (1) {
		// check for comma
		token nextToken;
		GET_TOKEN(nextToken, {});
		if (nextToken.type != TOKEN_COMMA) {
			unGetToken(&nextToken);
			break;
		}
		tokenDestroy(&nextToken);

		// parse next parameter
		astInputParameter param;
		TRY_PARSE(parseFunctionCallParameter(&param), {});
		astInputParameterListAdd(params, param);
	}

	return PARSE_OK;
}

static parseResult parseFunctionCall(astStatement* statement, const token* varName, const token* funcName) {
	statement->type = AST_STATEMENT_FUNC_CALL;

	TRY_PARSE(parseIdentifier(varName, &(statement->functionCall.varName)), {});
	TRY_PARSE(parseIdentifier(funcName, &(statement->functionCall.funcName)), {});
	TRY_PARSE(parseFunctionCallParams(&(statement->functionCall.params)), {});
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});

	return PARSE_OK;
}

static parseResult parseProcedureCall(astStatement* statement, const token* funcName) {
	statement->type = AST_STATEMENT_PROC_CALL;

	TRY_PARSE(parseIdentifier(funcName, &(statement->procedureCall.procName)), {});
	TRY_PARSE(parseFunctionCallParams(&(statement->procedureCall.params)), {});
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});

	return PARSE_OK;
}

static parseResult parseAssignment(astStatement* statement, const token* varName, token* exprFirstToken) {
	statement->type = AST_STATEMENT_ASSIGN;
	TRY_PARSE(parseIdentifier(varName, &(statement->assignment.variableName)), {});
	TRY_PARSE(parseExpression(&(statement->assignment.value), exprFirstToken), {});
	return PARSE_OK;
}

static parseResult parseAssignmentOrFunctionCall(astStatement* statement, const token* varName) {
	token firstToken;
	GET_TOKEN(firstToken, {});

	if (firstToken.type == TOKEN_IDENTIFIER) {
		token nextToken;
		GET_TOKEN(nextToken, { tokenDestroy(&firstToken); });

		if (nextToken.type == TOKEN_BRACKET_ROUND_LEFT) {
			tokenDestroy(&nextToken);
			TRY_PARSE(parseFunctionCall(statement, varName, &firstToken), { tokenDestroy(&firstToken); });
		} else {
			unGetToken(&nextToken);
			TRY_PARSE(parseAssignment(statement, varName, &firstToken), { tokenDestroy(&firstToken); });
		}
	} else {
		TRY_PARSE(parseAssignment(statement, varName, &firstToken), { tokenDestroy(&firstToken); });
	}

	tokenDestroy(&firstToken);

	return PARSE_OK;
}

static parseResult parseIteration(astStatement* statement, bool insideFunction) {
	statement->type = AST_STATEMENT_ITER;
	token exprFirstToken;
	GET_TOKEN(exprFirstToken, {});

	TRY_PARSE(parseExpression(&statement->iteration.condition, &exprFirstToken), { tokenDestroy(&exprFirstToken); });
	tokenDestroy(&exprFirstToken);

	TRY_PARSE(parseStatementBlock(&statement->iteration.body, insideFunction), {});
	return PARSE_OK;
}

static parseResult parseReturnStatement(astStatement* statement, bool withValue) {
	statement->type = AST_STATEMENT_RETURN;
	statement->returnStmt.hasValue = withValue;

	if (withValue) {
		token exprFirstToken;
		GET_TOKEN(exprFirstToken, {});
		TRY_PARSE(parseExpression(&(statement->returnStmt.value), &exprFirstToken), { tokenDestroy(&exprFirstToken); });
		tokenDestroy(&exprFirstToken);
	}

	return PARSE_OK;
}

static parseResult parseConditionalCondition(astCondition* condition) {
	token conditionFirstToken;
	GET_TOKEN(conditionFirstToken, {});

	if (conditionFirstToken.type == TOKEN_KEYWORD_LET) {
		// optional binding
		condition->type = AST_CONDITION_OPT_BINDING;
		token varNameToken;
		GET_TOKEN_ASSUME_TYPE(varNameToken, TOKEN_IDENTIFIER, { tokenDestroy(&conditionFirstToken); });
		TRY_PARSE(parseIdentifier(&varNameToken, &(condition->optBinding.identifier)), {
			tokenDestroy(&conditionFirstToken);
			tokenDestroy(&varNameToken);
		});
		tokenDestroy(&varNameToken);
	} else {
		// expression
		condition->type = AST_CONDITION_EXPRESSION;
		TRY_PARSE(parseExpression(&(condition->expression), &conditionFirstToken),
				  { tokenDestroy(&conditionFirstToken); });
	}

	tokenDestroy(&conditionFirstToken);

	return PARSE_OK;
}

static parseResult parseConditional(astStatement* statement, bool insideFunction) {
	statement->type = AST_STATEMENT_COND;

	// parse condition
	TRY_PARSE(parseConditionalCondition(&(statement->conditional.condition)), {});
	TRY_PARSE(parseStatementBlock(&(statement->conditional.body), insideFunction), {});

	token maybeElseToken;
	GET_TOKEN(maybeElseToken, {});
	if (maybeElseToken.type == TOKEN_KEYWORD_ELSE) {
		statement->conditional.hasElse = true;
		TRY_PARSE(parseStatementBlock(&(statement->conditional.bodyElse), insideFunction),
				  { tokenDestroy(&maybeElseToken); });
		tokenDestroy(&maybeElseToken);
	} else {
		statement->conditional.hasElse = false;
		unGetToken(&maybeElseToken);
	}

	return PARSE_OK;
}

static parseResult parseStatement(astStatement* statement, const token* firstToken, bool insideFunction) {
	assert(statement);
	assert(firstToken);

	switch (firstToken->type) {
		case TOKEN_KEYWORD_VAR:
			TRY_PARSE(parseVarDef(statement, false), {});
			break;
		case TOKEN_KEYWORD_LET:
			TRY_PARSE(parseVarDef(statement, true), {});
			break;
		case TOKEN_KEYWORD_RETURN:
			TRY_PARSE(parseReturnStatement(statement, insideFunction), {});
			break;
		case TOKEN_KEYWORD_IF:
			TRY_PARSE(parseConditional(statement, insideFunction), {});
			break;
		case TOKEN_KEYWORD_WHILE:
			TRY_PARSE(parseIteration(statement, insideFunction), {});
			break;
		case TOKEN_IDENTIFIER: {
			tokenType secondTokenType;
			GET_TOKEN_TYPE(secondTokenType, {});
			if (secondTokenType == TOKEN_ASSIGN) {
				TRY_PARSE(parseAssignmentOrFunctionCall(statement, firstToken), {});
			} else if (secondTokenType == TOKEN_BRACKET_ROUND_LEFT) {
				TRY_PARSE(parseProcedureCall(statement, firstToken), {});
			} else {
				return PARSE_ERROR;
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

static parseResult parseParameter(astParameter* param) {
	// parse outside name
	token outsideNameToken;
	GET_TOKEN(outsideNameToken, {});
	if (outsideNameToken.type == TOKEN_UNDERSCORE) {
		param->requiresName = false;
	} else if (outsideNameToken.type == TOKEN_IDENTIFIER) {
		param->requiresName = true;
		TRY_PARSE(parseIdentifier(&outsideNameToken, &(param->outsideName)), { tokenDestroy(&outsideNameToken); });
	} else {
		// TODO - emit message
		return PARSE_ERROR;
	}
	tokenDestroy(&outsideNameToken);

	// parse inside name
	token insideNameToken;
	GET_TOKEN_ASSUME_TYPE(insideNameToken, TOKEN_IDENTIFIER, {});
	TRY_PARSE(parseIdentifier(&insideNameToken, &(param->insideName)), { tokenDestroy(&insideNameToken); });

	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_COLON, {});

	// parse type
	TRY_PARSE(parseDataType(&(param->dataType)), {});

	return PARSE_OK;
}

static parseResult parseParameterList(astParameterList* list) {
	astParameterListCreate(list);

	astParameter firstParam;
	TRY_PARSE(parseParameter(&firstParam), {});
	astParameterListAdd(list, firstParam);

	while (1) {
		// check for comma
		token nextToken;
		GET_TOKEN(nextToken, {});
		if (nextToken.type != TOKEN_COMMA) {
			unGetToken(&nextToken);
			break;
		}
		tokenDestroy(&nextToken);

		// parse next parameter
		astParameter param;
		TRY_PARSE(parseParameter(&param), {});
		astParameterListAdd(list, param);
	}

	return PARSE_OK;
}

static parseResult parseFunctionDefinition(astFunctionDefinition* def) {
	// parse name
	token idToken;
	GET_TOKEN(idToken, {});
	TRY_PARSE(parseIdentifier(&idToken, &(def->name)), { tokenDestroy(&idToken); });
	tokenDestroy(&idToken);

	// parse params
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_LEFT, {});
	token maybeParamToken;
	GET_TOKEN(maybeParamToken, {});
	if (maybeParamToken.type != TOKEN_BRACKET_ROUND_RIGHT) {
		unGetToken(&maybeParamToken);
		TRY_PARSE(parseParameterList(&(def->params)), {});
		CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});
	} else {
		tokenDestroy(&maybeParamToken);
		astParameterListCreate(&(def->params));
	}

	// parse return type
	token maybeArrow;
	GET_TOKEN(maybeArrow, {});
	if (maybeArrow.type == TOKEN_ARROW) {
		def->hasReturnValue = true;
		tokenDestroy(&maybeArrow);
		TRY_PARSE(parseDataType(&(def->returnType)), {});
	} else {
		def->hasReturnValue = false;
		unGetToken(&maybeArrow);
	}

	// parse body
	TRY_PARSE(parseStatementBlock(&(def->body), def->hasReturnValue), {});

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
			TRY_PARSE(parseFunctionDefinition(&topStatement.functionDef), { tokenDestroy(&nextToken); });
		} else {
			topStatement.type = AST_TOP_STATEMENT;
			TRY_PARSE(parseStatement(&topStatement.statement, &nextToken, false), { tokenDestroy(&nextToken); });
		}

		tokenDestroy(&nextToken);

		if (astProgramAdd(program, topStatement) != 0) {
			return PARSE_INTERNAL_ERROR;
		}
	}

	return PARSE_OK;
}
