#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void astProgramCreate(astProgram *program) {
	program->statements = NULL;
	program->count = 0;
}

int astProgramAdd(astProgram *program, astTopLevelStatement statement) {
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

void astProgramDestroy(astProgram *program) {
	// TODO - recurse down the program and clear all nodes
	if (program->statements) {
		free(program->statements);
		program->statements = NULL;
	}
	program->count = 0;
}

int astBinaryExprCreate(astExpression *expr, astExpression lhs, astExpression rhs, astBinaryOperator op) {
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

void astStatementBlockCreate(astStatementBlock *block) {
	block->statements = NULL;
	block->count = 0;
}

int astStatementBlockAdd(astStatementBlock *block, astStatement statement) {
	if (block->statements == NULL) {
		block->statements = malloc(sizeof(statement));
	} else {
		block->statements = realloc(block->statements, sizeof(statement) * block->count + 1);
	}

	if (block->statements == NULL) {
		return 1;
	}

	block->statements[block->count++] = statement;
	return 0;
}

void astParameterListCreate(astParameterList *list) {
	list->data = NULL;
	list->count = 0;
}

int astParameterListAdd(astParameterList *list, astParameter param) {
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

void astInputParameterListCreate(astInputParameterList *list) {
	list->data = NULL;
	list->count = 0;
}

int astInputParameterListAdd(astInputParameterList *list, astInputParameter param) {
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

/*
void astBinaryExprDestroy(astExpression* expr) {
	free(expr->binary.lhs);
	free(expr->binary.rhs);
	expr->binary.lhs = NULL;
	expr->binary.rhs = NULL;
}
*/
