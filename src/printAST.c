#include "printAST.h"

#include <stdio.h>

#include "ast.h"

// forward declaration
void printStatement(const astStatement* statement, int indent);

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
			printf("IDENTIFIER: %s\n", term->identifier.name);
			break;
		case AST_TERM_NIL:
			puts("NIL");
			break;
	}
}

void printBinaryOperator(astBinaryOperator op) {
	switch (op) {
		case AST_BINARY_MUL:
			printf("*");
			break;
		case AST_BINARY_DIV:
			printf("/");
			break;
		case AST_BINARY_PLUS:
			printf("+");
			break;
		case AST_BINARY_MINUS:
			printf("-");
			break;
		case AST_BINARY_EQ:
			printf("==");
			break;
		case AST_BINARY_NEQ:
			printf("!=");
			break;
		case AST_BINARY_LESS:
			printf("<");
			break;
		case AST_BINARY_GREATER:
			printf(">");
			break;
		case AST_BINARY_LESS_EQ:
			printf("<=");
			break;
		case AST_BINARY_GREATER_EQ:
			printf(">=");
			break;
		case AST_BINARY_NIL_COAL:
			printf("??");
			break;
	}
}

void printExpression(const astExpression* expression, int indent);	// fwd

void printBinaryExpression(const astBinaryExpression* expression, int indent) {
	printIndent(indent);
	printf("BINARY EXPRESSION (");
	printBinaryOperator(expression->op);
	puts(")");
	printIndent(indent + 1);
	puts("LHS:");
	printExpression(expression->lhs, indent + 2);
	printIndent(indent + 1);
	puts("RHS:");
	printExpression(expression->rhs, indent + 2);
}

void printExpression(const astExpression* expression, int indent) {
	switch (expression->type) {
		case AST_EXPR_TERM:
			printTerm(&expression->term, indent);
			break;
		case AST_EXPR_BINARY:
			printBinaryExpression(&expression->binary, indent);
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
	printf("TYPE: ");
	printDataType(&definition->variableType);
	puts("");
	printIndent(indent + 1);
	puts("VALUE:");
	printExpression(&definition->value, indent + 2);
}

void printAssignment(const astAssignment* assignment, int indent) {
	printIndent(indent);
	puts("ASSIGNMENT:");
	printIndent(indent + 1);
	printf("NAME: %s\n", assignment->variableName);
	printIndent(indent + 1);
	puts("VALUE:");
	printExpression(&assignment->value, indent + 2);
}

void printStatementBlock(const astStatementBlock* block, int indent) {
	printIndent(indent);
	puts("{");
	for (int i = 0; i < block->count; i++) {
		printStatement(&(block->statements[i]), indent + 1);
	}
	printIndent(indent);
	puts("}");
}

void printConditional(const astConditional* conditional, int indent) {
	printIndent(indent);
	puts("CONDITIONAL:");
	printIndent(indent + 1);
	puts("CONDITION:");
	if (conditional->condition.type == AST_CONDITION_OPT_BINDING) {
		printIndent(indent + 2);
		printf("OPTIONAL BINDING: %s\n", conditional->condition.optBinding.identifier.name);
	} else {
		printExpression(&(conditional->condition.expression), indent + 2);
	}
	printIndent(indent + 1);
	puts("BODY:");
	printStatementBlock(&(conditional->body), indent + 2);
	if (conditional->has_else) {
		printIndent(indent + 1);
		puts("ELSE:");
		printStatementBlock(&(conditional->bodyElse), indent + 2);
	}
}

void printIteration(const astIteration* iteration, int indent) {
	printIndent(indent);
	puts("ITERATION");
	printIndent(indent + 1);
	puts("CONDITION:");
	printExpression(&iteration->condition, indent + 2);
	printIndent(indent + 1);
	puts("BODY:");
	printStatementBlock(&iteration->body, indent + 2);
}

void printFunctionCall(const astFunctionCall* call, int indent) {
	(void)call;
	(void)indent;
	// TODO
}

void printProcedureCall(const astProcedureCall* call, int indent) {
	(void)call;
	(void)indent;
	// TODO
}

void printReturn(const astReturnStatement* ret, int indent) {
	printIndent(indent);
	puts("RETURN");
	if (ret->has_value) {
		printIndent(indent + 1);
		puts("VALUE:");
		printExpression(&ret->value, indent + 2);
	}
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
		case AST_STATEMENT_PROC_CALL:
			printProcedureCall(&statement->procedureCall, indent);
			break;
		case AST_STATEMENT_RETURN:
			printReturn(&statement->returnStmt, indent);
			break;
	}
}

void printTopLevelStatement(const astTopLevelStatement* statement) {
	if (statement->type == AST_TOP_STATEMENT) {
		printStatement(&statement->statement, 0);
	} else {
		// printFunctionDef(&statement->functionDef);
	}
}

void astPrint(const astProgram* program) {
	for (int i = 0; i < program->count; i++) {
		printTopLevelStatement(&program->statements[i]);
	}
}
