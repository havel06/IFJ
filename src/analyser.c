#include "analyser.h"

#include <assert.h>
#include <stdio.h>

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

bool isNumberType(astDataType type) { return (type.type == AST_TYPE_INT || type.type == AST_TYPE_DOUBLE); }

bool isNoNullNumberType(astDataType type) { return !type.nullable && isNumberType(type); }

static analysisResult analyseBinaryExpression(const astBinaryExpression* expression, astDataType* outType) {
	// TODO
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
	} else if (!slot->variable.initialised) {
		fprintf(stderr, "Usage of uninitialised variable %s\n", id->name);
		return ANALYSIS_UNDEFINED_VAR;
	}

	*outType = slot->variable.type;
	return ANALYSIS_OK;
}

static analysisResult analyseExpression(const astExpression* expression, astDataType* outType) {
	// TODO
	switch (expression->type) {
		case AST_EXPR_TERM: {
			switch (expression->term.type) {
				case AST_TERM_ID:
					ANALYSE(analyseVariableId(&expression->term.identifier, outType), {});
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
	// check if function of same name is defined
	symbolTableSlot* slot = symTableLookup(&FUNC_SYM_TABLE, def->name.name);
	if (slot) {
		fprintf(stderr, "Redefinition of function %s\n", def->name.name);
		return ANALYSIS_UNDEFINED_FUNC;	 // TODO - is this the correct error value?
	}

	// add function to symbol table
	symbolFunc newSymbol = {def->name.name};
	symTableInsertFunc(&FUNC_SYM_TABLE, newSymbol, def->name.name);

	// TODO
	ANALYSE(analyseStatementBlock(&def->body), {});
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

	// insert into symtable
	symbolVariable newVar = {definition->variableType, definition->immutable, definition->hasInitValue};
	symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), newVar, definition->variableName.name);

	if (definition->hasInitValue) {
		astDataType initValueType;
		ANALYSE(analyseExpression(&definition->value, &initValueType), {});

		// TODO - this is probably not enough
		if (definition->variableType.type != initValueType.type) {
			fprintf(stderr, "Wrong type in initialisation of variable %s\n", definition->variableName.name);
			return ANALYSIS_WRONG_BINARY_TYPES;
		}
	}
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
	if (slot->variable.type.type != valueType.type) {
		fprintf(stderr, "Wrong type in assignment to variable %s\n", slot->name);
		return ANALYSIS_WRONG_BINARY_TYPES;
	}
	// TODO - type checking

	return ANALYSIS_OK;
}

// static analysisResult analyseOptionalBinding(const astOptionalBinding* binding) {
//	// TODO
//	return ANALYSIS_OK;
// }

static analysisResult analyseCondition(const astCondition* condition) {
	// TODO
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
		// ANALYSE(analyseOptionalBinding(&condition->optBinding));
	}
	return ANALYSIS_OK;
}

static analysisResult analyseConditional(const astConditional* conditional) {
	// TODO
	ANALYSE(analyseCondition(&conditional->condition), {});
	ANALYSE(analyseStatementBlock(&conditional->body), {});
	if (conditional->hasElse) {
		ANALYSE(analyseStatementBlock(&conditional->bodyElse), {});
	}
	return ANALYSIS_OK;
}

static analysisResult analyseIteration(const astIteration* iteration) {
	// TODO

	astDataType conditionType;
	ANALYSE(analyseExpression(&iteration->condition, &conditionType), {});
	if (conditionType.type != AST_TYPE_BOOL) {
		fputs("Condition must be of boolean type.", stderr);
		return ANALYSIS_WRONG_BINARY_TYPES;
	}
	if (conditionType.nullable) {
		fputs("Condition must not be nullable.", stderr);
		return ANALYSIS_WRONG_BINARY_TYPES;
	}

	ANALYSE(analyseStatementBlock(&iteration->body), {});
	return ANALYSIS_OK;
}

// static analysisResult analyseInputParameter(const astInputParameter* parameter) {
//	// TODO
//	(void)parameter;
//	return ANALYSIS_OK;
// }

static analysisResult analyseInputParameterList(const astInputParameterList* list) {
	(void)list;
	// TODO
	// for (int i = 0; i < list->count; i++) {
	//	ANALYSE(analyseInputParameter(&(list->data[i])));
	//}
	return ANALYSIS_OK;
}

static analysisResult analyseFunctionCall(const astFunctionCall* call) {
	// TODO

	ANALYSE(analyseInputParameterList(&call->params), {});
	return ANALYSIS_OK;
}

static analysisResult analyseProcedureCall(const astProcedureCall* call) {
	// TODO

	ANALYSE(analyseInputParameterList(&call->params), {});
	return ANALYSIS_OK;
}

static analysisResult analyseReturn(const astReturnStatement* ret) {
	// TODO

	if (ret->hasValue) {
		astDataType returnType;
		ANALYSE(analyseExpression(&ret->value, &returnType), {});
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
	symStackPop(&VAR_SYM_STACK);
	return ANALYSIS_OK;
}

analysisResult analyseProgram(const astProgram* program) {
	symTableCreate(&FUNC_SYM_TABLE);  // for functions
	symStackCreate(&VAR_SYM_STACK);
	symStackPush(&VAR_SYM_STACK);  // global scope
	// TODO - pop global scope and destroy symbol stack after usage
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_STATEMENT) {
			ANALYSE(analyseStatement(&topStatement->statement), {});
		} else {
			ANALYSE(analyseFunctionDef(&topStatement->functionDef), {});
		}
	}
	return ANALYSIS_OK;
}
