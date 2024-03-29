/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

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

// gets token from lexer to variable 'tokenVar'.
// If lexer returns an error, the macro calls return with an error value and calls 'onError' block
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

// gets token type from lexer to variable 'typeVar'.
// If lexer returns an error, the macro calls return with an error value and calls 'onError' block
#define GET_TOKEN_TYPE(typeVar, onError) \
	do {                                 \
		token testedToken;               \
		GET_TOKEN(testedToken, onError); \
		typeVar = testedToken.type;      \
		tokenDestroy(&testedToken);      \
	} while (0)

// gets token from lexer to variable 'typeVar'.
// If the token type does not match 'assumedType', the macro calls return with an error value.
// If lexer returns an error, the macro calls return with an error value and calls 'onError' block
#define GET_TOKEN_ASSUME_TYPE(tokenVar, assumedType, onError) \
	do {                                                      \
		GET_TOKEN(tokenVar, onError);                         \
		if (tokenVar.type != assumedType) {                   \
			onError;                                          \
			return PARSE_ERROR;                               \
		}                                                     \
	} while (0)

// Consumes a token from lexer, calling 'onError' if an error occurs.
#define CONSUME_TOKEN(onError)             \
	do {                                   \
		token consumedToken;               \
		GET_TOKEN(consumedToken, onError); \
		tokenDestroy(&consumedToken);      \
	} while (0)

// Consumes a token from lexer, calling 'onError' if an error occurs.
// If the token type does not match 'assumedType', the macro calls return with an error value.
#define CONSUME_TOKEN_ASSUME_TYPE(assumedType, onError)             \
	do {                                                            \
		token consumedToken;                                        \
		GET_TOKEN_ASSUME_TYPE(consumedToken, assumedType, onError); \
		tokenDestroy(&consumedToken);                               \
	} while (0)

// Calls 'function' (must be of return type 'parseResult').
// If the called function returns with an error, the macro calls return with the respective error value and calls
// 'onError'.
#define TRY_PARSE(function, onError) \
	do {                             \
		int retval = function;       \
		if (retval != 0) {           \
			onError;                 \
			return retval;           \
		}                            \
	} while (0)

// Converts token type to its corresponding AST data type.
static parseResult keywordToDataType(tokenType type, astBasicDataType* output) {
	switch (type) {
		case TOKEN_KEYWORD_INT:
			*output = AST_TYPE_INT;
			return PARSE_OK;

		case TOKEN_KEYWORD_DOUBLE:
			*output = AST_TYPE_DOUBLE;
			return PARSE_OK;

		case TOKEN_KEYWORD_STRING:
			*output = AST_TYPE_STRING;
			return PARSE_OK;
		default:
			return PARSE_ERROR;
	}
}

// tok = first token
static void parseIntLiteral(const token* tok, astTerm* term) {
	term->type = AST_TERM_INT;
	term->integer.value = atoi(tok->content);
}

// tok = first token
static void parseDecimalLiteral(const token* tok, astTerm* term) {
	term->type = AST_TERM_DECIMAL;
	term->decimal.value = atof(tok->content);
}

// tok = first token
static parseResult parseStringLiteral(const token* tok, astTerm* term) {
	term->type = AST_TERM_STRING;
	term->string.content = malloc(strlen(tok->content) * sizeof(char) + 1);
	if (!term->string.content) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(term->string.content, tok->content);

	return PARSE_OK;
}

// tok = first token
static parseResult parseIdentifier(const token* tok, astIdentifier* identifier) {
	identifier->name = malloc(strlen(tok->content) + 1);
	if (!identifier->name) {
		return PARSE_INTERNAL_ERROR;
	}
	strcpy(identifier->name, tok->content);
	return PARSE_OK;
}

// tok = first token
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

// Primary expression = (expression) | term | unwrap expression
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

// Higher number = higher precedence
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

	TRY_PARSE(parsePrimaryExpression(&subExpressions[subExpressionsCount++], exprFirstToken), {});

	while (true) {
		token firstToken;
		GET_TOKEN(firstToken, {});

		if (!isBinaryOperator(firstToken.type)) {
			unGetToken(&firstToken);
			break;
		}

		operators[operatorCount++] = parseBinaryOperator(firstToken.type);
		tokenDestroy(&firstToken);

		token subExprFirstToken;
		GET_TOKEN(subExprFirstToken, {});
		TRY_PARSE(parsePrimaryExpression(&subExpressions[subExpressionsCount++], &subExprFirstToken),
				  { tokenDestroy(&subExprFirstToken); });
		tokenDestroy(&subExprFirstToken);
	}

	while (operatorCount > 0) {
		// apply operator precedence
		int maxPrecedence = 0;
		int maxPrecedenceIndex = 0;
		// scan operators for highest precedence
		for (int i = 0; i < operatorCount; i++) {
			int precedence = operatorPrecedence(operators[i]);
			if (precedence > maxPrecedence) {
				maxPrecedence = precedence;
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
		for (int i = maxPrecedenceIndex; i < operatorCount - 1; i++) {
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
	TRY_PARSE(keywordToDataType(tok.type, &(dataType->type)), { tokenDestroy(&tok); });
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

static parseResult parseStatementBlock(astStatementBlock* block, bool insideFunction) {
	astStatementBlockCreate(block);
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_CURLY_LEFT, {});
	while (true) {
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

// Starting left bracket is assumed to be consumed
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

	while (true) {
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
		if (astInputParameterListAdd(params, param)) {
			return PARSE_INTERNAL_ERROR;
		}
	}

	return PARSE_OK;
}

// Opening left bracket of parameter list is assumed to be consumed
static parseResult parseFunctionCall(astFunctionCall* call, const token* varName, const token* funcName) {
	TRY_PARSE(parseIdentifier(varName, &(call->varName)), {});
	TRY_PARSE(parseIdentifier(funcName, &(call->funcName)), {});
	TRY_PARSE(parseFunctionCallParams(&(call->params)), {});
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});

	return PARSE_OK;
}

// Opening left bracket of parameter list is assumed to be consumed
static parseResult parseProcedureCall(astStatement* statement, const token* funcName) {
	statement->type = AST_STATEMENT_PROC_CALL;

	TRY_PARSE(parseIdentifier(funcName, &(statement->procedureCall.procName)), {});
	TRY_PARSE(parseFunctionCallParams(&(statement->procedureCall.params)), {});
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_BRACKET_ROUND_RIGHT, {});

	return PARSE_OK;
}

// '=' has already been consumed
static parseResult parseAssignment(astStatement* statement, const token* varName, token* exprFirstToken) {
	statement->type = AST_STATEMENT_ASSIGN;
	TRY_PARSE(parseIdentifier(varName, &(statement->assignment.variableName)), {});
	TRY_PARSE(parseExpression(&(statement->assignment.value), exprFirstToken), {});
	return PARSE_OK;
}

// Founc and identifier (varName), now we must determine whether it is an assignment or a function call
static parseResult parseAssignmentOrFunctionCall(astStatement* statement, const token* varName) {
	token firstToken;
	GET_TOKEN(firstToken, {});

	if (firstToken.type == TOKEN_IDENTIFIER) {
		token nextToken;
		GET_TOKEN(nextToken, { tokenDestroy(&firstToken); });

		if (nextToken.type == TOKEN_BRACKET_ROUND_LEFT) {
			tokenDestroy(&nextToken);
			statement->type = AST_STATEMENT_FUNC_CALL;
			TRY_PARSE(parseFunctionCall(&statement->functionCall, varName, &firstToken),
					  { tokenDestroy(&firstToken); });
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

// Variable initialiser = function call or an expression
static parseResult parseVarInit(astVariableInitialiser* initialiser, const char* varName) {
	token firstToken;
	GET_TOKEN(firstToken, {});

	token secondToken;
	GET_TOKEN(secondToken, { tokenDestroy(&firstToken); });

	if (firstToken.type == TOKEN_IDENTIFIER && secondToken.type == TOKEN_BRACKET_ROUND_LEFT) {
		// function call

		// NOTE - we create a fake token, since parseFunctionCall expects a token, not a string
		token varNameToken = {TOKEN_IDENTIFIER, (char*)varName};

		initialiser->type = AST_VAR_INIT_FUNC;
		TRY_PARSE(parseFunctionCall(&initialiser->call, &varNameToken, &firstToken), {
			tokenDestroy(&firstToken);
			tokenDestroy(&secondToken);
		});
		tokenDestroy(&secondToken);
	} else {
		// expression
		initialiser->type = AST_VAR_INIT_EXPR;
		unGetToken(&secondToken);
		TRY_PARSE(parseExpression(&(initialiser->expr), &firstToken), { tokenDestroy(&firstToken); });
	}

	tokenDestroy(&firstToken);
	return PARSE_OK;
}

static parseResult parseVarDef(astStatement* statement, bool immutable) {
	statement->type = AST_STATEMENT_VAR_DEF;
	statement->variableDef.immutable = immutable;

	token variableNameToken;
	GET_TOKEN_ASSUME_TYPE(variableNameToken, TOKEN_IDENTIFIER, {});
	TRY_PARSE(parseIdentifier(&variableNameToken, &(statement->variableDef.variableName)),
			  { tokenDestroy(&variableNameToken); });
	tokenDestroy(&variableNameToken);

	// parse variable type
	token maybeColonToken;
	GET_TOKEN(maybeColonToken, {});
	if (maybeColonToken.type == TOKEN_COLON) {
		statement->variableDef.hasExplicitType = true;
		tokenDestroy(&maybeColonToken);
		TRY_PARSE(parseDataType(&(statement->variableDef.variableType)),
				  { free(statement->variableDef.variableName.name); });
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
	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_ASSIGN, {});
	statement->variableDef.hasInitValue = true;
	TRY_PARSE(parseVarInit(&statement->variableDef.value, statement->variableDef.variableName.name),
			  { free(statement->variableDef.variableName.name); });

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

// parameter inside function declaration
static parseResult parseParameter(astParameter* param) {
	// parse outside name
	token outsideNameToken;
	GET_TOKEN(outsideNameToken, {});
	if (outsideNameToken.type == TOKEN_UNDERSCORE) {
		param->requiresName = false;
		param->outsideName.name = "UNNAMED";
	} else if (outsideNameToken.type == TOKEN_IDENTIFIER) {
		param->requiresName = true;
		TRY_PARSE(parseIdentifier(&outsideNameToken, &(param->outsideName)), { tokenDestroy(&outsideNameToken); });
	} else {
		return PARSE_ERROR;
	}
	tokenDestroy(&outsideNameToken);

	// parse inside name
	token insideNameToken;
	GET_TOKEN(insideNameToken, {});
	if (insideNameToken.type == TOKEN_UNDERSCORE) {
		param->used = false;
		param->insideName.name = "UNUSED";
	} else if (insideNameToken.type == TOKEN_IDENTIFIER) {
		param->used = true;
		TRY_PARSE(parseIdentifier(&insideNameToken, &(param->insideName)), { tokenDestroy(&insideNameToken); });
	} else {
		tokenDestroy(&insideNameToken);
		return PARSE_ERROR;
	}
	tokenDestroy(&insideNameToken);

	CONSUME_TOKEN_ASSUME_TYPE(TOKEN_COLON, {});

	// parse type
	TRY_PARSE(parseDataType(&(param->dataType)), {});

	return PARSE_OK;
}

// parameter list of function declaration
// opening left bracket has already been consumed
static parseResult parseParameterList(astParameterList* list) {
	astParameterListCreate(list);

	astParameter firstParam;
	TRY_PARSE(parseParameter(&firstParam), {});
	if (astParameterListAdd(list, firstParam)) {
		return PARSE_INTERNAL_ERROR;
	}

	while (true) {
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
		if (astParameterListAdd(list, param)) {
			return PARSE_INTERNAL_ERROR;
		}
	}

	return PARSE_OK;
}

// func keyword has already been consumed
static parseResult parseFunctionDefinition(astFunctionDefinition* def) {
	// parse name
	token idToken;
	GET_TOKEN_ASSUME_TYPE(idToken, TOKEN_IDENTIFIER, {});
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

	while (true) {
		GET_TOKEN(nextToken, {});
		astTopLevelStatement topStatement;

		if (nextToken.type == TOKEN_EOF) {
			break;
		} else if (nextToken.type == TOKEN_KEYWORD_FUNC) {
			topStatement.type = AST_TOP_FUNCTION;
			TRY_PARSE(parseFunctionDefinition(&topStatement.functionDef), { tokenDestroy(&nextToken); });
		} else if (nextToken.type == TOKEN_KEYWORD_RETURN) {
			// Return statements are forbidden in global scope
			tokenDestroy(&nextToken);
			return PARSE_ERROR;
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
