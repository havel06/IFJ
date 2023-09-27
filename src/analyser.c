#include "analyser.h"

#include "ast.h"

#define ANALYSE(func)                     \
	do {                                  \
		analysisResult funcRetVal = func; \
		if (funcRetVal != ANALYSIS_OK) {  \
			return funcRetVal;            \
		}                                 \
	} while (0)

// forward decl
static analysisResult analyseStatementBlock(const astStatementBlock*);
static analysisResult analyseExpression(const astExpression*);

static analysisResult analyseBinaryExpression(const astBinaryExpression* expression) {
	// TODO
	ANALYSE(analyseExpression(expression->lhs));
	ANALYSE(analyseExpression(expression->rhs));
	return ANALYSIS_OK;
}

static analysisResult analyseUnwrapExpression(const astUnwrapExpression* expression) {
	// TODO
	ANALYSE(analyseExpression(expression->innerExpr));
	return ANALYSIS_OK;
}

static analysisResult analyseExpression(const astExpression* expression) {
	// TODO
	switch (expression->type) {
		case AST_EXPR_TERM:
			break;
		case AST_EXPR_BINARY:
			ANALYSE(analyseBinaryExpression(&expression->binary));
			break;
		case AST_EXPR_UNWRAP:
			ANALYSE(analyseUnwrapExpression(&expression->unwrap));
			break;
	}
	return ANALYSIS_OK;
}

static analysisResult analyseFunctionDef(const astFunctionDefinition* statement) {
	// TODO
	ANALYSE(analyseStatementBlock(&statement->body));
	return ANALYSIS_OK;
}

static analysisResult analyseVariableDef(const astVariableDefinition* definition) {
	// TODO
	if (definition->hasInitValue) {
		ANALYSE(analyseExpression(&definition->value));
	}
	return ANALYSIS_OK;
}

static analysisResult analyseAssignment(const astAssignment* assignment) {
	// TODO
	ANALYSE(analyseExpression(&assignment->value));
	return ANALYSIS_OK;
}

// static analysisResult analyseOptionalBinding(const astOptionalBinding* binding) {
//	// TODO
//	return ANALYSIS_OK;
// }

static analysisResult analyseCondition(const astCondition* condition) {
	// TODO
	if (condition->type == AST_CONDITION_EXPRESSION) {
		ANALYSE(analyseExpression(&condition->expression));
	} else {
		// ANALYSE(analyseOptionalBinding(&condition->optBinding));
	}
	return ANALYSIS_OK;
}

static analysisResult analyseConditional(const astConditional* conditional) {
	// TODO
	ANALYSE(analyseCondition(&conditional->condition));
	ANALYSE(analyseStatementBlock(&conditional->body));
	if (conditional->hasElse) {
		ANALYSE(analyseStatementBlock(&conditional->bodyElse));
	}
	return ANALYSIS_OK;
}

static analysisResult analyseIteration(const astIteration* iteration) {
	// TODO

	ANALYSE(analyseExpression(&iteration->condition));
	ANALYSE(analyseStatementBlock(&iteration->body));
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

	ANALYSE(analyseInputParameterList(&call->params));
	return ANALYSIS_OK;
}

static analysisResult analyseProcedureCall(const astProcedureCall* call) {
	// TODO

	ANALYSE(analyseInputParameterList(&call->params));
	return ANALYSIS_OK;
}

static analysisResult analyseReturn(const astReturnStatement* ret) {
	// TODO

	if (ret->hasValue) {
		ANALYSE(analyseExpression(&ret->value));
	}

	return ANALYSIS_OK;
}

static analysisResult analyseStatement(const astStatement* statement) {
	switch (statement->type) {
		case AST_STATEMENT_VAR_DEF:
			ANALYSE(analyseVariableDef(&statement->variableDef));
			break;
		case AST_STATEMENT_ASSIGN:
			ANALYSE(analyseAssignment(&statement->assignment));
			break;
		case AST_STATEMENT_COND:
			ANALYSE(analyseConditional(&statement->conditional));
			break;
		case AST_STATEMENT_ITER:
			ANALYSE(analyseIteration(&statement->iteration));
			break;
		case AST_STATEMENT_FUNC_CALL:
			ANALYSE(analyseFunctionCall(&statement->functionCall));
			break;
		case AST_STATEMENT_PROC_CALL:
			ANALYSE(analyseProcedureCall(&statement->procedureCall));
			break;
		case AST_STATEMENT_RETURN:
			ANALYSE(analyseReturn(&statement->returnStmt));
			break;
	}

	return ANALYSIS_OK;
}

static analysisResult analyseStatementBlock(const astStatementBlock* block) {
	for (int i = 0; i < block->count; i++) {
		ANALYSE(analyseStatement(&block->statements[i]));
	}
	return ANALYSIS_OK;
}

analysisResult analyseProgram(const astProgram* program) {
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_STATEMENT) {
			ANALYSE(analyseStatement(&topStatement->statement));
		} else {
			ANALYSE(analyseFunctionDef(&topStatement->functionDef));
		}
	}
	return ANALYSIS_OK;
}
