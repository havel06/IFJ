#include "analyser.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "symtable.h"

#define ANALYSE(func, onError)            \
	do {                                  \
		analysisResult funcRetVal = func; \
		if (funcRetVal != ANALYSIS_OK) {  \
			onError;                      \
			return funcRetVal;            \
		}                                 \
	} while (0)

// forward decl
static analysisResult analyseStatementBlock(const astStatementBlock*);
static analysisResult analyseExpression(const astExpression*, astDataType* outType);

static symbolTableStack VAR_SYM_STACK;
static symbolTable FUNC_SYM_TABLE;
static const astFunctionDefinition* CURRENT_FUNCTION;

// builtin functions
static astParameterList EMPTY_PARAMS;
static astParameterList INT2DOUBLE_PARAMS;
static astParameterList DOUBLE2INT_PARAMS;
static astParameterList LENGTH_PARAMS;
static astParameterList SUBSTRING_PARAMS;
static astParameterList ORD_PARAMS;
static astParameterList CHR_PARAMS;

// checks if type is easily convertible (nil to nullable, nonull to nullable)
bool isTriviallyConvertible(astDataType to, astDataType from) {
	if (to.nullable && from.type == AST_TYPE_NIL) {
		return true;
	}

	if (!to.nullable && from.nullable) {
		return false;
	}

	if (!to.nullable && from.type == AST_TYPE_NIL) {
		return false;
	}

	return to.type == from.type;
}

bool isNumberType(astDataType type) { return (type.type == AST_TYPE_INT || type.type == AST_TYPE_DOUBLE); }

bool isNoNullNumberType(astDataType type) { return !type.nullable && isNumberType(type); }

static analysisResult analyseBinaryExpression(const astBinaryExpression* expression, astDataType* outType) {
	astDataType lhsType;
	astDataType rhsType;
	ANALYSE(analyseExpression(expression->lhs, &lhsType), {});
	ANALYSE(analyseExpression(expression->rhs, &rhsType), {});

	switch (expression->op) {
		case AST_BINARY_PLUS:
			if (lhsType.type == AST_TYPE_STRING && rhsType.type == AST_TYPE_STRING) {
				outType->type = AST_TYPE_STRING;
				break;
			}
			__attribute__((fallthrough));
		case AST_BINARY_MUL:
		case AST_BINARY_MINUS:
			if (!isNoNullNumberType(lhsType) || !isNoNullNumberType(rhsType)) {
				fputs("Incompatible types for binary operation.", stderr);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}
			if (lhsType.type == AST_TYPE_INT && rhsType.type == AST_TYPE_INT) {
				outType->type = AST_TYPE_INT;
			} else {
				outType->type = AST_TYPE_DOUBLE;
			}
			outType->nullable = false;
			break;
		case AST_BINARY_DIV:
			if (!isNoNullNumberType(lhsType) || !isNoNullNumberType(rhsType)) {
				fputs("Incompatible types for binary operation.", stderr);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}
			outType->type = AST_TYPE_DOUBLE;
			outType->nullable = false;
			break;
		case AST_BINARY_LESS_EQ:
		case AST_BINARY_GREATER_EQ:
		case AST_BINARY_LESS:
		case AST_BINARY_GREATER:
			if (!isNoNullNumberType(lhsType) || !isNoNullNumberType(rhsType)) {
				fputs("Incompatible types for binary operation.", stderr);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}
			if (lhsType.type != rhsType.type) {
				fputs("Incompatible types for binary operation.", stderr);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}
			outType->type = AST_TYPE_BOOL;
			outType->nullable = false;
			break;
		case AST_BINARY_EQ:
		case AST_BINARY_NEQ:
			if ((lhsType.type != rhsType.type) && !(isNumberType(lhsType) && isNumberType(rhsType))) {
				fputs("Incompatible types for binary operation.", stderr);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}
			outType->type = AST_TYPE_BOOL;
			outType->nullable = lhsType.nullable || rhsType.nullable;
			break;
		case AST_BINARY_NIL_COAL:
			if (!lhsType.nullable) {
				fputs("Left side of nil coalescing operator must be nullable.", stderr);
				return ANALYSIS_OTHER_ERROR;  // TODO - is this correct?
			}
			if (lhsType.type != rhsType.type) {
				return ANALYSIS_WRONG_BINARY_TYPES;	 // TODO - is this correct?
			}
			break;
	}

	return ANALYSIS_OK;
}

static analysisResult analyseUnwrapExpression(const astUnwrapExpression* expression, astDataType* outType) {
	ANALYSE(analyseExpression(expression->innerExpr, outType), {});

	if (!outType->nullable) {
		fputs("Cannot unwrap non-nullable value.", stderr);
		return ANALYSIS_OTHER_ERROR;
	}

	outType->nullable = false;
	return ANALYSIS_OK;
}

static analysisResult analyseVariableId(const astIdentifier* id, astDataType* outType) {
	symbolTableSlot* slot = symStackLookup(&VAR_SYM_STACK, id->name, NULL);
	if (!slot) {
		fprintf(stderr, "Usage of undefined variable %s\n", id->name);
		return ANALYSIS_UNDEFINED_VAR;
	} else if (!slot->variable.initialisedInScope) {
		fprintf(stderr, "Usage of uninitialised variable %s\n", id->name);
		return ANALYSIS_UNDEFINED_VAR;
	}

	*outType = slot->variable.type;
	return ANALYSIS_OK;
}

static analysisResult analyseTerm(const astTerm* term, astDataType* outType) {
	switch (term->type) {
		case AST_TERM_ID:
			ANALYSE(analyseVariableId(&term->identifier, outType), {});
			break;
		case AST_TERM_INT:
			outType->type = AST_TYPE_INT;
			outType->nullable = false;
			break;
		case AST_TERM_DECIMAL:
			outType->type = AST_TYPE_DOUBLE;
			outType->nullable = false;
			break;
		case AST_TERM_NIL:
			outType->type = AST_TYPE_NIL;
			outType->nullable = false;
			break;
		case AST_TERM_STRING:
			outType->type = AST_TYPE_STRING;
			outType->nullable = false;
			break;
	}

	return ANALYSIS_OK;
}

static analysisResult analyseExpression(const astExpression* expression, astDataType* outType) {
	switch (expression->type) {
		case AST_EXPR_TERM: {
			ANALYSE(analyseTerm(&expression->term, outType), {});
			break;
		}
		case AST_EXPR_BINARY:
			ANALYSE(analyseBinaryExpression(&expression->binary, outType), {});
			break;
		case AST_EXPR_UNWRAP:
			ANALYSE(analyseUnwrapExpression(&expression->unwrap, outType), {});
			break;
	}
	return ANALYSIS_OK;
}

static analysisResult analyseFunctionDef(const astFunctionDefinition* def) {
	CURRENT_FUNCTION = def;
	symStackPush(&VAR_SYM_STACK);
	// add params to scope
	for (int i = 0; i < def->params.count; i++) {
		astParameter* param = &def->params.data[i];
		symbolVariable symbol = {param->dataType, true, symStackCurrentScope(&VAR_SYM_STACK)};
		symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), symbol, param->insideName.name);
	}
	ANALYSE(analyseStatementBlock(&def->body), {});
	symStackPush(&VAR_SYM_STACK);
	CURRENT_FUNCTION = NULL;
	return ANALYSIS_OK;
}

static analysisResult analyseVariableDef(const astVariableDefinition* definition) {
	// check for variable redefinition
	symbolTable* scopePtr;
	symbolTableSlot* slot = symStackLookup(&VAR_SYM_STACK, definition->variableName.name, &scopePtr);
	if (slot && scopePtr == symStackCurrentScope(&VAR_SYM_STACK)) {
		// redefined
		fprintf(stderr, "Variable redefinition: %s\n", definition->variableName.name);
		return ANALYSIS_UNDEFINED_FUNC;
	}

	astDataType variableType = definition->variableType;

	if (definition->hasInitValue) {
		astDataType initValueType;
		ANALYSE(analyseExpression(&definition->value, &initValueType), {});

		if (definition->hasExplicitType) {
			if (initValueType.type == AST_TYPE_NIL) {
				fprintf(stderr, "Cannot deduce nil type in initialisation of variable %s\n",
						definition->variableName.name);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}

			if (!isTriviallyConvertible(definition->variableType, initValueType)) {
				fprintf(stderr, "Wrong type in initialisation of variable %s\n", definition->variableName.name);
				return ANALYSIS_WRONG_BINARY_TYPES;
			}
		} else {
			variableType = initValueType;
		}
	}

	// insert into symtable
	symbolVariable newVar = {variableType, definition->immutable,
							 definition->hasInitValue ? symStackCurrentScope(&VAR_SYM_STACK) : NULL};
	symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), newVar, definition->variableName.name);

	return ANALYSIS_OK;
}

static analysisResult analyseAssignment(const astAssignment* assignment) {
	symbolTableSlot* slot = symStackLookup(&VAR_SYM_STACK, assignment->variableName.name, NULL);
	// check if variable exists
	if (!slot) {
		fprintf(stderr, "Usage of undefined variable %s\n", assignment->variableName.name);
		return ANALYSIS_UNDEFINED_VAR;
	}
	// check if variable is mutable
	if (slot->variable.immutable) {
		fprintf(stderr, "Modification of immutable variable %s\n", assignment->variableName.name);
		return ANALYSIS_OTHER_ERROR;  // TODO - is this correct?
	}

	astDataType valueType;
	ANALYSE(analyseExpression(&assignment->value, &valueType), {});
	if (!isTriviallyConvertible(slot->variable.type, valueType)) {
		fprintf(stderr, "Wrong type in assignment to variable %s\n", slot->name);
		return ANALYSIS_WRONG_BINARY_TYPES;
	}

	slot->variable.initialisedInScope = symStackCurrentScope(&VAR_SYM_STACK);

	return ANALYSIS_OK;
}

static analysisResult analyseOptionalBinding(const astOptionalBinding* binding) {
	astDataType variableType;
	ANALYSE(analyseVariableId(&binding->identifier, &variableType), {});
	if (!variableType.nullable) {
		fputs("Variable used in optional binding must be nullable.\n", stderr);
		return ANALYSIS_OTHER_ERROR;  // TODO - is this correct?
	}
	// TODO - add new variable to if statement block
	return ANALYSIS_OK;
}

static analysisResult analyseCondition(const astCondition* condition) {
	if (condition->type == AST_CONDITION_EXPRESSION) {
		astDataType conditionType;
		ANALYSE(analyseExpression(&condition->expression, &conditionType), {});
		if (conditionType.type != AST_TYPE_BOOL) {
			fputs("Condition must be of boolean type.", stderr);
			return ANALYSIS_WRONG_BINARY_TYPES;
		}
		if (conditionType.nullable) {
			fputs("Condition must not be nullable.", stderr);
			return ANALYSIS_WRONG_BINARY_TYPES;
		}
	} else {
		ANALYSE(analyseOptionalBinding(&condition->optBinding), {});
	}
	return ANALYSIS_OK;
}

static analysisResult analyseConditional(const astConditional* conditional) {
	ANALYSE(analyseCondition(&conditional->condition), {});
	ANALYSE(analyseStatementBlock(&conditional->body), {});
	if (conditional->hasElse) {
		ANALYSE(analyseStatementBlock(&conditional->bodyElse), {});
	}
	return ANALYSIS_OK;
}

static analysisResult analyseIteration(const astIteration* iteration) {
	astDataType conditionType;
	ANALYSE(analyseExpression(&iteration->condition, &conditionType), {});
	if (conditionType.type != AST_TYPE_BOOL) {
		fputs("Condition must be of boolean type.\n", stderr);
		return ANALYSIS_WRONG_BINARY_TYPES;
	}
	if (conditionType.nullable) {
		fputs("Condition must not be nullable.\n", stderr);
		return ANALYSIS_WRONG_BINARY_TYPES;
	}

	ANALYSE(analyseStatementBlock(&iteration->body), {});
	return ANALYSIS_OK;
}

static analysisResult analyseInputParameterList(const astParameterList* list, const astInputParameterList* input) {
	if (list->count != input->count) {
		fputs("Wrong number of parameters.\n", stderr);
		return ANALYSIS_WRONG_FUNC_TYPE;  // TODO - is this correct?
	}

	for (int i = 0; i < list->count; i++) {
		astParameter* param = &list->data[i];
		astInputParameter* inParam = &input->data[i];
		if (param->requiresName) {
			if (!inParam->hasName) {
				fprintf(stderr, "Parameter %s requires to be called explicitely.\n", param->outsideName.name);
				return ANALYSIS_WRONG_FUNC_TYPE;  // TODO - is this correct?
			}

			if (strcmp(param->outsideName.name, inParam->name.name) != 0) {
				fprintf(stderr, "Parameter names %s and %s don't match.\n", param->outsideName.name,
						inParam->name.name);
				return ANALYSIS_WRONG_FUNC_TYPE;
			}
		} else if (inParam->hasName) {
			fputs("Parameter does not require a name in function call.\n", stderr);
			return ANALYSIS_WRONG_FUNC_TYPE;  // TODO - is this correct?
		}

		astDataType inParamType;
		ANALYSE(analyseTerm(&inParam->value, &inParamType), {});
		if (!isTriviallyConvertible(param->dataType, inParamType)) {
			fprintf(stderr, "Wrong type passed to parameter %s\n", param->outsideName.name);
			return ANALYSIS_WRONG_FUNC_TYPE;
		}
	}

	return ANALYSIS_OK;
}

static analysisResult analyseFunctionCall(const astFunctionCall* call) {
	// check if function exists
	if (strcmp(call->funcName.name, "write") != 0) {
		symbolTableSlot* slot = symTableLookup(&FUNC_SYM_TABLE, call->funcName.name);
		if (!slot) {
			fprintf(stderr, "Calling undefined function %s\n", call->funcName.name);
			return ANALYSIS_UNDEFINED_FUNC;
		}

		ANALYSE(analyseInputParameterList(slot->function.params, &call->params), {});
	}
	// TODO - check if return type matches

	return ANALYSIS_OK;
}

static analysisResult analyseProcedureCall(const astProcedureCall* call) {
	// check if function exists
	if (strcmp(call->procName.name, "write") != 0) {
		symbolTableSlot* slot = symTableLookup(&FUNC_SYM_TABLE, call->procName.name);
		if (!slot) {
			fprintf(stderr, "Calling undefined function %s\n", call->procName.name);
			return ANALYSIS_UNDEFINED_FUNC;
		}

		ANALYSE(analyseInputParameterList(slot->function.params, &call->params), {});
	}

	return ANALYSIS_OK;
}

static analysisResult analyseReturn(const astReturnStatement* ret) {
	if (ret->hasValue) {
		astDataType returnType;
		ANALYSE(analyseExpression(&ret->value, &returnType), {});

		if (!isTriviallyConvertible(CURRENT_FUNCTION->returnType, returnType)) {
			fputs("Incompatible return type.\n", stderr);
			return ANALYSIS_WRONG_RETURN;
		}
	}

	return ANALYSIS_OK;
}

static analysisResult analyseStatement(const astStatement* statement) {
	switch (statement->type) {
		case AST_STATEMENT_VAR_DEF:
			ANALYSE(analyseVariableDef(&statement->variableDef), {});
			break;
		case AST_STATEMENT_ASSIGN:
			ANALYSE(analyseAssignment(&statement->assignment), {});
			break;
		case AST_STATEMENT_COND:
			ANALYSE(analyseConditional(&statement->conditional), {});
			break;
		case AST_STATEMENT_ITER:
			ANALYSE(analyseIteration(&statement->iteration), {});
			break;
		case AST_STATEMENT_FUNC_CALL:
			ANALYSE(analyseFunctionCall(&statement->functionCall), {});
			break;
		case AST_STATEMENT_PROC_CALL:
			ANALYSE(analyseProcedureCall(&statement->procedureCall), {});
			break;
		case AST_STATEMENT_RETURN:
			ANALYSE(analyseReturn(&statement->returnStmt), {});
			break;
	}

	return ANALYSIS_OK;
}

static analysisResult analyseStatementBlock(const astStatementBlock* block) {
	symStackPush(&VAR_SYM_STACK);
	for (int i = 0; i < block->count; i++) {
		ANALYSE(analyseStatement(&block->statements[i]), {});
	}

	// uninitialise variables initialised in this scope
	symbolTable* scope = symStackCurrentScope(&VAR_SYM_STACK);
	for (int t = 0; t < VAR_SYM_STACK.count; t++) {
		for (int i = 0; i < SYM_TABLE_CAPACITY; i++) {
			symbolTableSlot* slot = &VAR_SYM_STACK.tables[t].data[i];
			if (slot->taken && slot->variable.initialisedInScope == scope) {
				slot->variable.initialisedInScope = NULL;
			}
		}
	}

	symStackPop(&VAR_SYM_STACK);
	return ANALYSIS_OK;
}

static analysisResult registerFunction(const astFunctionDefinition* def) {
	// check if function of same name is defined
	symbolTableSlot* slot = symTableLookup(&FUNC_SYM_TABLE, def->name.name);
	if (slot) {
		fprintf(stderr, "Redefinition of function %s\n", def->name.name);
		return ANALYSIS_UNDEFINED_FUNC;	 // TODO - is this the correct error value?
	}

	// add function to symbol table
	symbolFunc newSymbol = {&def->params};
	symTableInsertFunc(&FUNC_SYM_TABLE, newSymbol, def->name.name);
	return ANALYSIS_OK;
}

static void registerReadString() {
	symbolFunc symbol = {&EMPTY_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "readString");
}

static void registerReadInt() {
	symbolFunc symbol = {&EMPTY_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "readInt");
}

static void registerReadDouble() {
	symbolFunc symbol = {&EMPTY_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "readDouble");
}

static void registerInt2Double() {
	astParameterListCreate(&INT2DOUBLE_PARAMS);
	astParameter term;
	term.dataType.type = AST_TYPE_INT;
	term.dataType.nullable = false;
	term.requiresName = false;
	term.outsideName.name = NULL;
	term.insideName.name = NULL;
	astParameterListAdd(&INT2DOUBLE_PARAMS, term);

	symbolFunc symbol = {&INT2DOUBLE_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "Int2Double");
}

static void registerDouble2Int() {
	astParameterListCreate(&DOUBLE2INT_PARAMS);
	astParameter term;
	term.dataType.type = AST_TYPE_DOUBLE;
	term.dataType.nullable = false;
	term.requiresName = false;
	term.outsideName.name = NULL;
	term.insideName.name = NULL;
	astParameterListAdd(&DOUBLE2INT_PARAMS, term);

	symbolFunc symbol = {&DOUBLE2INT_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "Double2Int");
}

static void registerLength() {
	astParameterListCreate(&LENGTH_PARAMS);
	astParameter s;
	s.dataType.type = AST_TYPE_STRING;
	s.dataType.nullable = false;
	s.requiresName = false;
	s.outsideName.name = NULL;
	s.insideName.name = NULL;
	astParameterListAdd(&LENGTH_PARAMS, s);

	symbolFunc symbol = {&LENGTH_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "Double2Int");
}

static void registerSubstring() {
	astParameterListCreate(&SUBSTRING_PARAMS);

	astParameter of;
	of.dataType.type = AST_TYPE_STRING;
	of.dataType.nullable = false;
	of.requiresName = true;
	of.outsideName.name = "of";
	of.insideName.name = NULL;
	astParameterListAdd(&SUBSTRING_PARAMS, of);

	astParameter startingAt;
	startingAt.dataType.type = AST_TYPE_INT;
	startingAt.dataType.nullable = false;
	startingAt.requiresName = true;
	startingAt.outsideName.name = "startingAt";
	startingAt.insideName.name = NULL;
	astParameterListAdd(&SUBSTRING_PARAMS, startingAt);

	astParameter endingBefore;
	endingBefore.dataType.type = AST_TYPE_INT;
	endingBefore.dataType.nullable = false;
	endingBefore.requiresName = true;
	endingBefore.outsideName.name = "endingBefore";
	endingBefore.insideName.name = NULL;
	astParameterListAdd(&SUBSTRING_PARAMS, endingBefore);

	symbolFunc symbol = {&SUBSTRING_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "substring");
}

static void registerOrd() {
	astParameterListCreate(&ORD_PARAMS);
	astParameter c;
	c.dataType.type = AST_TYPE_STRING;
	c.dataType.nullable = false;
	c.requiresName = false;
	c.outsideName.name = NULL;
	c.insideName.name = NULL;
	astParameterListAdd(&ORD_PARAMS, c);

	symbolFunc symbol = {&ORD_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "ord");
}

static void registerChr() {
	astParameterListCreate(&CHR_PARAMS);
	astParameter i;
	i.dataType.type = AST_TYPE_INT;
	i.dataType.nullable = false;
	i.requiresName = false;
	i.outsideName.name = NULL;
	i.insideName.name = NULL;
	astParameterListAdd(&CHR_PARAMS, i);

	symbolFunc symbol = {&CHR_PARAMS};
	symTableInsertFunc(&FUNC_SYM_TABLE, symbol, "ord");
}

static void registerBuiltinFunctions() {
	astParameterListCreate(&EMPTY_PARAMS);

	registerReadString();
	registerReadDouble();
	registerReadInt();
	registerInt2Double();
	registerDouble2Int();
	registerLength();
	registerSubstring();
	registerOrd();
	registerChr();
}

static void cleanUpBuiltinFunctions() {
	astParameterListDestroyNoRecurse(&EMPTY_PARAMS);
	astParameterListDestroyNoRecurse(&INT2DOUBLE_PARAMS);
	astParameterListDestroyNoRecurse(&DOUBLE2INT_PARAMS);
	astParameterListDestroyNoRecurse(&LENGTH_PARAMS);
	astParameterListDestroyNoRecurse(&SUBSTRING_PARAMS);
	astParameterListDestroyNoRecurse(&ORD_PARAMS);
	astParameterListDestroyNoRecurse(&CHR_PARAMS);
}

analysisResult analyseProgram(const astProgram* program) {
	symTableCreate(&FUNC_SYM_TABLE);  // for functions
	registerBuiltinFunctions();
	symStackCreate(&VAR_SYM_STACK);
	symStackPush(&VAR_SYM_STACK);  // global scope
	// TODO - pop global scope and destroy symbol stack after usage

	// first pass - register all functions
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_FUNCTION) {
			ANALYSE(registerFunction(&topStatement->functionDef), {});
		}
	}

	// second pass - analyse statements and function bodies
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_STATEMENT) {
			ANALYSE(analyseStatement(&topStatement->statement), {});
		} else {
			ANALYSE(analyseFunctionDef(&topStatement->functionDef), {});
		}
	}

	cleanUpBuiltinFunctions();

	return ANALYSIS_OK;
}
