/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void astProgramCreate(astProgram* program) {
	program->statements = NULL;
	program->count = 0;
}

int astProgramAdd(astProgram* program, astTopLevelStatement statement) {
	if (program->statements == NULL) {
		// allocate memory for first statement
		program->statements = malloc(sizeof(statement));
		if (program->statements != NULL) {
			program->count = 1;
			program->statements[0] = statement;
			return 0;
		} else {
			return 1;
		}
	}

	// push new statement to array
	program->statements = realloc(program->statements, (program->count + 1) * sizeof(statement));
	if (program->statements == NULL) {
		return 1;
	}

	program->statements[program->count] = statement;
	program->count++;
	return 0;
}

int astBinaryExprCreate(astExpression* expr, astExpression lhs, astExpression rhs, astBinaryOperator op) {
	expr->type = AST_EXPR_BINARY;

	expr->binary.lhs = malloc(sizeof(astExpression));
	if (!expr->binary.lhs) {
		return 1;
	}
	expr->binary.rhs = malloc(sizeof(astExpression));
	if (!expr->binary.rhs) {
		free(expr->binary.lhs);
		return 1;
	}

	*(expr->binary.lhs) = lhs;
	*(expr->binary.rhs) = rhs;
	expr->binary.op = op;
	return 0;
}

void astStatementBlockCreate(astStatementBlock* block) {
	block->statements = NULL;
	block->count = 0;
}

int astStatementBlockAdd(astStatementBlock* block, astStatement statement) {
	if (block->statements == NULL) {
		block->statements = malloc(sizeof(statement));
	} else {
		block->statements = realloc(block->statements, sizeof(statement) * (block->count + 1));
	}

	if (block->statements == NULL) {
		return 1;
	}

	block->statements[block->count++] = statement;
	return 0;
}

void astParameterListCreate(astParameterList* list) {
	list->data = NULL;
	list->count = 0;
}

int astParameterListAdd(astParameterList* list, astParameter param) {
	if (list->data == NULL) {
		list->data = malloc(sizeof(param));
	} else {
		list->data = realloc(list->data, sizeof(param) * (list->count + 1));
	}

	if (list->data == NULL) {
		return 1;
	}

	list->data[list->count++] = param;
	return 0;
}

void astInputParameterListCreate(astInputParameterList* list) {
	list->data = NULL;
	list->count = 0;
}

int astInputParameterListAdd(astInputParameterList* list, astInputParameter param) {
	if (list->data == NULL) {
		list->data = malloc(sizeof(param));
	} else {
		list->data = realloc(list->data, sizeof(param) * (list->count + 1));
	}

	if (list->data == NULL) {
		return 1;
	}

	list->data[list->count++] = param;
	return 0;
}

static void astIdentifierDestroy(astIdentifier* identifier) {
	free(identifier->name);
	identifier->name = NULL;
}

static void astTermDestroy(astTerm* term) {
	if (term->type == AST_TERM_ID) {
		astIdentifierDestroy(&term->identifier);
	} else if (term->type == AST_TERM_STRING) {
		free(term->string.content);
		term->string.content = NULL;
	}
}

static void astExpressionDestroy(astExpression* expr) {
	switch (expr->type) {
		case AST_EXPR_TERM:
			astTermDestroy(&expr->term);
			break;
		case AST_EXPR_BINARY:
			astExpressionDestroy(expr->binary.lhs);
			astExpressionDestroy(expr->binary.rhs);
			free(expr->binary.lhs);
			free(expr->binary.rhs);
			expr->binary.lhs = NULL;
			expr->binary.rhs = NULL;
			break;
		case AST_EXPR_UNWRAP:
			astExpressionDestroy(expr->unwrap.innerExpr);
			expr->unwrap.innerExpr = NULL;
			break;
	}
}

static void astAssignmentDestroy(astAssignment* assignment) {
	astIdentifierDestroy(&assignment->variableName);
	astExpressionDestroy(&assignment->value);
}

static void astConditionalDestroy(astConditional* conditional) {
	if (conditional->condition.type == AST_CONDITION_EXPRESSION) {
		astExpressionDestroy(&conditional->condition.expression);
	} else {
		astIdentifierDestroy(&conditional->condition.optBinding.identifier);
	}

	astStatementBlockDestroy(&conditional->body);
	if (conditional->hasElse) {
		astStatementBlockDestroy(&conditional->bodyElse);
	}
}

static void astInputParameterListDestroy(astInputParameterList* list) {
	for (int i = 0; i < list->count; i++) {
		astInputParameter* param = &list->data[i];
		if (param->hasName) {
			astIdentifierDestroy(&param->name);
		}
		astTermDestroy(&param->value);
	}

	if (list->data) {
		free(list->data);
	}
}

static void astFunctionCallDestroy(astFunctionCall* call) {
	astIdentifierDestroy(&call->varName);
	astIdentifierDestroy(&call->funcName);
	astInputParameterListDestroy(&call->params);
}

static void astProcedureCallDestroy(astProcedureCall* call) {
	astIdentifierDestroy(&call->procName);
	astInputParameterListDestroy(&call->params);
}

static void astVariableDefinitionDestroy(astVariableDefinition* def) {
	astIdentifierDestroy(&def->variableName);
	if (def->hasInitValue) {
		if (def->value.type == AST_VAR_INIT_EXPR) {
			astExpressionDestroy(&def->value.expr);
		} else {
			astFunctionCallDestroy(&def->value.call);
		}
	}
}

static void astStatementDestroy(astStatement* statement) {
	switch (statement->type) {
		case AST_STATEMENT_ASSIGN:
			astAssignmentDestroy(&statement->assignment);
			break;
		case AST_STATEMENT_VAR_DEF:
			astVariableDefinitionDestroy(&statement->variableDef);
			break;
		case AST_STATEMENT_RETURN:
			if (statement->returnStmt.hasValue) {
				astExpressionDestroy(&statement->returnStmt.value);
			}
			break;
		case AST_STATEMENT_COND:
			astConditionalDestroy(&statement->conditional);
			break;
		case AST_STATEMENT_ITER:
			astExpressionDestroy(&statement->iteration.condition);
			astStatementBlockDestroy(&statement->iteration.body);
			break;
		case AST_STATEMENT_FUNC_CALL:
			astFunctionCallDestroy(&statement->functionCall);
			break;
		case AST_STATEMENT_PROC_CALL:
			astProcedureCallDestroy(&statement->procedureCall);
			break;
	}
}

void astStatementBlockDestroy(astStatementBlock* block) {
	if (block->statements) {
		for (int i = 0; i < block->count; i++) {
			astStatementDestroy(&block->statements[i]);
		}
		free(block->statements);
	}
	block->statements = 0;
}

void astParameterListDestroyNoRecurse(astParameterList* list) {
	if (list->data) {
		free(list->data);
	}
}

void astParameterListDestroy(astParameterList* list) {
	for (int i = 0; i < list->count; i++) {
		astParameter* param = &list->data[i];
		if (param->requiresName) {
			astIdentifierDestroy(&param->outsideName);
		}
		astIdentifierDestroy(&param->insideName);
	}

	astParameterListDestroyNoRecurse(list);
}

static void astFunctionDefinitionDestroy(astFunctionDefinition* def) {
	astIdentifierDestroy(&def->name);
	astParameterListDestroy(&def->params);
	astStatementBlockDestroy(&def->body);
}

void astProgramDestroy(astProgram* program) {
	if (program->statements) {
		for (int i = 0; i < program->count; i++) {
			astTopLevelStatement* top = &program->statements[i];
			if (top->type == AST_TOP_STATEMENT) {
				astStatementDestroy(&top->statement);
			} else {
				astFunctionDefinitionDestroy(&top->functionDef);
			}
		}

		free(program->statements);
		program->statements = NULL;
	}
	program->count = 0;
}
