#include "compiler.h"

#include <assert.h>
#include <stdio.h>

#include "ast.h"

// 0 = global
static int FRAME_LEVEL = 0;

// forward decl
static astDataType compileExpression(const astExpression*);

static void emitVariableId(const astIdentifier* var) {
	if (FRAME_LEVEL == 0) {
		printf("GF@%s", var->name);
	} else {
		printf("LF@%s", var->name);
	}
}

static astDataType compileTerm(const astTerm* term) {
	astDataType dataType;
	dataType.nullable = false;

	switch (term->type) {
		case AST_TERM_ID:
			printf("PUSHS ");
			emitVariableId(&term->identifier);
			puts("");
			// TODO - find type in symbol table
			dataType.type = AST_TYPE_NIL;
			break;
		case AST_TERM_INT:
			printf("PUSHS int@%d\n", term->integer.value);
			dataType.type = AST_TYPE_INT;
			break;
		case AST_TERM_DECIMAL:
			printf("PUSHS float@%a\n", term->decimal.value);
			dataType.type = AST_TYPE_DOUBLE;
			break;
		case AST_TERM_STRING:
			// TODO - escape sequences, control characters
			printf("PUSHS string@%s\n", term->string.content);
			dataType.type = AST_TYPE_STRING;
			break;
		case AST_TERM_NIL:
			puts("PUSHS nil@nil");
			dataType.type = AST_TYPE_NIL;
			break;
	}

	return dataType;
}

static astDataType compileBinaryExpression(const astBinaryExpression* expr) {
	astDataType lhsType = compileExpression(expr->lhs);
	astDataType rhsType = compileExpression(expr->rhs);
	puts("CREATEFRAME");
	puts("DEFVAR TF@lhs");
	puts("DEFVAR TF@rhs");
	puts("POPS TF@rhs");
	puts("POPS TF@lhs");

	astDataType resultType;
	resultType.nullable = false;

	// perform conversion
	if (lhsType.type == rhsType.type) {
		// do nothing
	} else if (lhsType.type == AST_TYPE_INT) {
		puts("INT2FLOAT TF@lhs TF@lhs");
	} else if (rhsType.type == AST_TYPE_INT) {
		puts("INT2FLOAT TF@rhs TF@rhs");
	}

	switch (expr->op) {
		case AST_BINARY_MUL:
			puts("MUL TF@res TF@lhs TF@rhs");
			resultType.type = lhsType.type;
			break;
		case AST_BINARY_DIV:
			if (lhsType.type == AST_TYPE_DOUBLE) {
				puts("DIV TF@res TF@lhs TF@rhs");
			} else {
				puts("IDIV TF@res TF@lhs TF@rhs");
			}
			resultType.type = lhsType.type;
			break;
		case AST_BINARY_PLUS:
			puts("ADD TF@res TF@lhs TF@rhs");
			resultType.type = lhsType.type;
			break;
		case AST_BINARY_MINUS:
			puts("SUB TF@res TF@lhs TF@rhs");
			resultType.type = lhsType.type;
			break;
		case AST_BINARY_EQ:
			puts("EQ TF@res TF@lhs TF@rhs");
			resultType.type = AST_TYPE_BOOL;
			break;
		case AST_BINARY_NEQ:
			puts("EQ TF@res TF@lhs TF@rhs");
			puts("NOT TF@res TF@res");
			resultType.type = AST_TYPE_BOOL;
			break;
		case AST_BINARY_LESS:
			puts("LT TF@res TF@lhs TF@rhs");
			resultType.type = AST_TYPE_BOOL;
			break;
		case AST_BINARY_GREATER:
			puts("GT TF@res TF@lhs TF@rhs");
			resultType.type = AST_TYPE_BOOL;
			break;
		case AST_BINARY_LESS_EQ:
			puts("GT TF@res TF@lhs TF@rhs");
			puts("NOT TF@res TF@res");
			resultType.type = AST_TYPE_BOOL;
			break;
		case AST_BINARY_GREATER_EQ:
			puts("LT TF@res TF@lhs TF@rhs");
			puts("NOT TF@res TF@res");
			break;
		case AST_BINARY_NIL_COAL:
			// TODO
			break;
	}
	puts("PUSHS TF@res");
	return resultType;
}

static astDataType compileExpression(const astExpression* expr) {
	switch (expr->type) {
		case AST_EXPR_TERM:
			return compileTerm(&expr->term);
			break;
		case AST_EXPR_BINARY:
			return compileBinaryExpression(&expr->binary);
			break;
		case AST_EXPR_UNWRAP:
			// TODO
			break;
	}
	assert(false);
}

static void compileAssignment(const astAssignment* assignment) {
	compileExpression(&assignment->value);
	printf("POPS ");
	emitVariableId(&assignment->variableName);
	puts("");
}

static void compileVariableDef(const astVariableDefinition* def) {
	printf("DEFVAR ");
	emitVariableId(&def->variableName);
	puts("");

	if (def->hasInitValue) {
		compileExpression(&def->value);
		printf("POPS ");
		emitVariableId(&def->variableName);
		puts("");
	}
}

static void compileStatement(const astStatement* statement) {
	switch (statement->type) {
		case AST_STATEMENT_VAR_DEF:
			compileVariableDef(&statement->variableDef);
			break;
		case AST_STATEMENT_ASSIGN:
			compileAssignment(&statement->assignment);
			break;
		case AST_STATEMENT_COND:
			// TODO
			// compileConditional(&statement->conditional);
			break;
		case AST_STATEMENT_ITER:
			// TODO
			// compileIteration(&statement->iteration);
			break;
		case AST_STATEMENT_FUNC_CALL:
			// TODO
			// compileFunctionCall(&statement->functionCall);
			break;
		case AST_STATEMENT_PROC_CALL:
			// TODO
			// compileProcedureCall(&statement->procedureCall);
			break;
		case AST_STATEMENT_RETURN:
			// TODO
			// compileReturn(&statement->returnStmt);
			break;
	}
}

void compileProgram(const astProgram* program) {
	puts(".IFJcode23");
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_STATEMENT) {
			compileStatement(&topStatement->statement);
		} else {
			// TODO
		}
	}
}
