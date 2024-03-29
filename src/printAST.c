/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#include "printAST.h"

#include <assert.h>
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
		case AST_TYPE_NIL:
			printf("nil");
			break;
		case AST_TYPE_BOOL:
			printf("bool");
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

void printBinaryOperator(astBinaryOperator op, FILE* f) {
	switch (op) {
		case AST_BINARY_MUL:
			fprintf(f, "*");
			break;
		case AST_BINARY_DIV:
			fprintf(f, "/");
			break;
		case AST_BINARY_PLUS:
			fprintf(f, "+");
			break;
		case AST_BINARY_MINUS:
			fprintf(f, "-");
			break;
		case AST_BINARY_EQ:
			fprintf(f, "==");
			break;
		case AST_BINARY_NEQ:
			fprintf(f, "!=");
			break;
		case AST_BINARY_LESS:
			fprintf(f, "<");
			break;
		case AST_BINARY_GREATER:
			fprintf(f, ">");
			break;
		case AST_BINARY_LESS_EQ:
			fprintf(f, "<=");
			break;
		case AST_BINARY_GREATER_EQ:
			fprintf(f, ">=");
			break;
		case AST_BINARY_NIL_COAL:
			fprintf(f, "??");
			break;
	}
}

void printExpression(const astExpression* expression, int indent);	// fwd

void printBinaryExpression(const astBinaryExpression* expression, int indent) {
	printIndent(indent);
	printf("BINARY EXPRESSION (");
	printBinaryOperator(expression->op, stdout);
	puts(")");
	printIndent(indent + 1);
	puts("LHS:");
	printExpression(expression->lhs, indent + 2);
	printIndent(indent + 1);
	puts("RHS:");
	printExpression(expression->rhs, indent + 2);
}

void printUnwrapExpression(const astUnwrapExpression* expr, int indent) {
	printIndent(indent);
	puts("UNWRAP:");
	printExpression(expr->innerExpr, indent + 1);
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
			printUnwrapExpression(&expression->unwrap, indent);
			break;
	}
}

void printAssignment(const astAssignment* assignment, int indent) {
	printIndent(indent);
	puts("ASSIGNMENT:");
	printIndent(indent + 1);
	printf("NAME: %s\n", assignment->variableName.name);
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
	if (conditional->hasElse) {
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

void printFunctionCallParams(const astInputParameterList* list, int indent) {
	printIndent(indent);
	puts("PARAMS:");
	for (int i = 0; i < list->count; i++) {
		printIndent(indent + 1);
		puts("PARAM:");
		if (list->data[i].hasName) {
			printIndent(indent + 2);
			printf("NAME: %s\n", list->data[i].name.name);
		}
		printIndent(indent + 2);
		puts("VALUE:");
		printTerm(&(list->data[i].value), indent + 3);
	}
}

void printFunctionCall(const astFunctionCall* call, int indent) {
	printIndent(indent);
	puts("FUNCTION CALL:");

	printIndent(indent + 1);
	printf("FUNCTION NAME: %s\n", call->funcName.name);

	printIndent(indent + 1);
	printf("VARIABLE NAME: %s\n", call->varName.name);

	printFunctionCallParams(&call->params, indent + 1);
}

void printProcedureCall(const astProcedureCall* call, int indent) {
	printIndent(indent);
	puts("PROCEDURE CALL:");

	printIndent(indent + 1);
	printf("PROCEDURE NAME: %s\n", call->procName.name);

	printFunctionCallParams(&call->params, indent + 1);
}

void printReturn(const astReturnStatement* ret, int indent) {
	printIndent(indent);
	puts("RETURN");
	if (ret->hasValue) {
		printIndent(indent + 1);
		puts("VALUE:");
		printExpression(&ret->value, indent + 2);
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
	printf("NAME: %s\n", definition->variableName.name);

	printIndent(indent + 1);
	printf("TYPE: ");

	if (definition->hasExplicitType) {
		printDataType(&definition->variableType);
		puts("");
	} else {
		puts("unspecified");
	}

	if (definition->hasInitValue) {
		printIndent(indent + 1);
		puts("VALUE:");
		if (definition->value.type == AST_VAR_INIT_EXPR) {
			printExpression(&definition->value.expr, indent + 2);
		} else {
			printFunctionCall(&definition->value.call, indent + 2);
		}
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

void printFunctionDef(const astFunctionDefinition* def) {
	puts("FUNCTION DEFINITION");
	printIndent(1);
	printf("NAME: %s\n", def->name.name);
	printIndent(1);
	puts("PARAMETERS:");
	for (int i = 0; i < def->params.count; i++) {
		printIndent(2);
		puts("PARAM");
		printIndent(3);
		printf("OUTSIDE_NAME: %s\n", def->params.data[i].outsideName.name);
		printIndent(3);
		printf("INSIDE_NAME: %s\n", def->params.data[i].insideName.name);
		printIndent(3);
		printf("TYPE: ");
		printDataType(&(def->params.data[i].dataType));
		puts("");
	}
	if (def->hasReturnValue) {
		printIndent(1);
		printf("RETURN TYPE: ");
		printDataType(&(def->returnType));
		puts("");
	}
	printIndent(1);
	puts("BODY:");
	printStatementBlock(&(def->body), 2);
}

void printTopLevelStatement(const astTopLevelStatement* statement) {
	if (statement->type == AST_TOP_STATEMENT) {
		printStatement(&statement->statement, 0);
	} else {
		printFunctionDef(&statement->functionDef);
	}
}

void astPrint(const astProgram* program) {
	for (int i = 0; i < program->count; i++) {
		printTopLevelStatement(&program->statements[i]);
	}
}
