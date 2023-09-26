#include "printAST.h"

#include <stdio.h>

#include "ast.h"

void printIndent(int level) {
	for (int i = 0; i < level * 2; i++) {
		putchar(' ');
	}
}

void printDataType(const astDataType* type) {
	switch (type->type) {
		case AST_TYPE_INT:
			printf("Int");
			break;
		case AST_TYPE_DOUBLE:
			printf("Double");
			break;
		case AST_TYPE_STRING:
			printf("String");
			break;
		case AST_TYPE_UNSPECIFIED:
			printf("Unspecified");
			break;
	}

	if (type->nullable) {
		printf(" (nullable)");
	}
}

void printTerm(const astTerm* term, int indent) {
	printIndent(indent);
	switch (term->type) {
		case AST_TERM_DECIMAL:
			printf("DECIMAL LITERAL: %lf\n", term->decimal.value);
			break;
		case AST_TERM_INT:
			printf("INTEGER LITERAL: %d\n", term->integer.value);
			break;
		case AST_TERM_STRING:
			printf("STRING LITERAL: %s\n", term->string.content);
			break;
		case AST_TERM_ID:
			printf("IDENTIFIER: %s\n", term->identificator.name);
			break;
	}
}

void printExpression(const astExpression* expression, int indent) {
	switch (expression->type) {
		case AST_EXPR_TERM:
			printTerm(&expression->term, indent);
			break;
		case AST_EXPR_BINARY:
			break;
		case AST_EXPR_UNWRAP:
			break;
	}
}

void printVariableDefinition(const astVariableDefinition* definition, int indent) {
	printIndent(indent);
	;
	printf("VARIABLE DEF: ");
	if (definition->immutable) {
		puts("(immutable)");
	} else {
		puts("(mutable)");
	}
	printIndent(indent + 1);
	printf("NAME: %s\n", definition->variableName);
	printIndent(indent + 1);
	puts("TYPE:");
	printDataType(&definition->variableType);
	puts("VALUE:");
	printExpression(&definition->value, indent + 1);
}

void printAssignment(const astAssignment* assignment, int indent) {
	(void)assignment;
	(void)indent;
	// TODO
}

void printConditional(const astConditional* conditional, int indent) {
	(void)conditional;
	(void)indent;
	// TODO
}

void printIteration(const astIteration* iteration, int indent) {
	(void)iteration;
	(void)indent;
	// TODO
}

void printFunctionCall(const astFunctionCall* call, int indent) {
	(void)call;
	(void)indent;
	// TODO
}

void printVoidFunctionCall(const astVoidFunctionCall* call, int indent) {
	(void)call;
	(void)indent;
	// TODO
}

void printReturn(const astReturnStatement* ret, int indent) {
	(void)ret;
	(void)indent;
	// TODO
}

void printStatement(const astStatement* statement, int indent) {
	switch (statement->type) {
		case AST_STATEMENT_VAR_DEF:
			printVariableDefinition(&statement->variableDef, indent);
			break;
		case AST_STATEMENT_ASSIGN:
			printAssignment(&statement->assignment, indent);
			break;
		case AST_STATEMENT_COND:
			printConditional(&statement->conditional, indent);
			break;
		case AST_STATEMENT_ITER:
			printIteration(&statement->iteration, indent);
			break;
		case AST_STATEMENT_FUNC_CALL:
			printFunctionCall(&statement->functionCall, indent);
			break;
		case AST_STATEMENT_FUNC_CALL_VOID:
			printVoidFunctionCall(&statement->voidFunctionCall, indent);
			break;
		case AST_STATEMENT_RETURN:
			printReturn(&statement->returnStmt, indent);
			break;
	}
}

void printTopLeveStatement(const astTopLevelStatement* statement) {
	if (statement->type == AST_TOP_STATEMENT) {
		printStatement(&statement->statement, 0);
	} else {
		// printFunctionDef(&statement->functionDef);
	}
}

void astPrint(const astProgram* program) {
	for (int i = 0; i < program->count; i++) {
		printTopLeveStatement(&program->statements[i]);
	}
}
