#include "analyser.h"

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
static analysisResult analyseExpression(const astExpression*);

static symbolTableStack VAR_SYM_STACK;

static analysisResult analyseBinaryExpression(const astBinaryExpression* expression) {
	// TODO
	ANALYSE(analyseExpression(expression->lhs), {});
	ANALYSE(analyseExpression(expression->rhs), {});
	return ANALYSIS_OK;
}

static analysisResult analyseUnwrapExpression(const astUnwrapExpression* expression) {
	// TODO
	ANALYSE(analyseExpression(expression->innerExpr), {});
	return ANALYSIS_OK;
}

static analysisResult analyseVariableId(const astIdentifier* id) {
	symbolTableSlot* slot = symStackLookup(&VAR_SYM_STACK, id->name, NULL);
	if (!slot) {
		fprintf(stderr, "Usage of undefined variable %s\n", id->name);
		return ANALYSIS_UNDEFINED_VAR;
	} else if (!slot->variable.initialised) {
		fprintf(stderr, "Usage of uninitialised variable %s\n", id->name);
		return ANALYSIS_UNDEFINED_VAR;
	}
	return ANALYSIS_OK;
}

static analysisResult analyseExpression(const astExpression* expression) {
	// TODO
	switch (expression->type) {
		case AST_EXPR_TERM:
			if (expression->term.type == AST_TERM_ID) {
				ANALYSE(analyseVariableId(&expression->term.identifier), {});
			}
			break;
		case AST_EXPR_BINARY:
			ANALYSE(analyseBinaryExpression(&expression->binary), {});
			break;
		case AST_EXPR_UNWRAP:
			ANALYSE(analyseUnwrapExpression(&expression->unwrap), {});
			break;
	}
	return ANALYSIS_OK;
}

static analysisResult analyseFunctionDef(const astFunctionDefinition* statement) {
	// TODO
	ANALYSE(analyseStatementBlock(&statement->body), {});
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
		ANALYSE(analyseExpression(&definition->value), {});
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

	ANALYSE(analyseExpression(&assignment->value), {});
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
		ANALYSE(analyseExpression(&condition->expression), {});
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

	ANALYSE(analyseExpression(&iteration->condition), {});
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
		ANALYSE(analyseExpression(&ret->value), {});
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
