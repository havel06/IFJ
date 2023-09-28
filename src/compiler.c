#include "compiler.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "symtable.h"

// 0 = global frame
static int FRAME_LEVEL = 0;
static symbolTableStack VAR_SYM_STACK;

#define PUSH_FRAME()              \
	puts("CREATEFRAME");          \
	puts("PUSHFRAME");            \
	symStackPush(&VAR_SYM_STACK); \
	FRAME_LEVEL++

#define POP_FRAME()              \
	puts("POPFRAME");            \
	symStackPop(&VAR_SYM_STACK); \
	FRAME_LEVEL--

// used for compiler-generated labes (in conditionals etc)
static int LAST_LABEL_NAME = 0;

static int newLabelName() { return LAST_LABEL_NAME++; }

// forward decl
static astDataType compileExpression(const astExpression*);
static void compileStatement(const astStatement*);

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
			symbolTableSlot* slot = symStackLookup(&VAR_SYM_STACK, term->identifier.name, NULL);
			dataType = slot->variable.type;
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
			puts("DIV TF@res TF@lhs TF@rhs");
			resultType.type = AST_TYPE_DOUBLE;
			break;
		case AST_BINARY_PLUS:
			if (lhsType.type == AST_TYPE_STRING) {
				puts("CONCAT TF@res TF@lhs TF@rhs");
			} else {
				puts("ADD TF@res TF@lhs TF@rhs");
			}
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
		case AST_BINARY_NIL_COAL: {
			int coalLabel = newLabelName();
			puts("MOVE TF@res TF@lhs");
			printf("JUMPIFNEQ %d TF@res nil@nil\n", coalLabel);
			puts("MOVE TF@res TF@rhs");
			printf("LABEL %d\n", coalLabel);
			break;
		}
	}
	puts("PUSHS TF@res");
	return resultType;
}

static astDataType compileUnwrapExpression(const astUnwrapExpression* expr) {
	return compileExpression(expr->innerExpr);
}

static astDataType compileExpression(const astExpression* expr) {
	switch (expr->type) {
		case AST_EXPR_TERM:
			return compileTerm(&expr->term);
		case AST_EXPR_BINARY:
			return compileBinaryExpression(&expr->binary);
		case AST_EXPR_UNWRAP:
			return compileUnwrapExpression(&expr->unwrap);
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
	// insert into symtable
	symbolVariable newVar = {def->variableType, def->immutable, def->hasInitValue};
	symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), newVar, def->variableName.name);

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

static void compileStatementBlock(const astStatementBlock* block) {
	PUSH_FRAME();
	for (int i = 0; i < block->count; i++) {
		compileStatement(&block->statements[i]);
	}
	POP_FRAME();
}

static void compileConditional(const astConditional* conditional) {
	if (conditional->condition.type == AST_CONDITION_EXPRESSION) {
		compileExpression(&conditional->condition.expression);
	} else {
		// TODO
		assert(false);
	}

	int label1 = newLabelName();
	puts("PUSHS bool@true");
	printf("JUMPIFNEQS %d\n", label1);

	compileStatementBlock(&conditional->body);

	if (!conditional->hasElse) {
		printf("LABEL %d\n", label1);
		puts("CLEARS");
		return;
	}

	int label2 = newLabelName();
	printf("JUMP %d\n", label2);
	printf("LABEL %d\n", label1);

	compileStatementBlock(&conditional->bodyElse);

	printf("LABEL %d\n", label2);
	puts("CLEARS");
}

static void compileIteration(const astIteration* iteration) {
	int startLabel = newLabelName();
	int condLabel = newLabelName();
	printf("JUMP %d\n", condLabel);
	printf("LABEL %d\n", startLabel);
	// body
	compileStatementBlock(&iteration->body);
	// condition
	printf("LABEL %d\n", condLabel);
	compileExpression(&iteration->condition);
	puts("PUSHS bool@true");
	printf("JUMPIFEQS %d\n", startLabel);
	puts("CLEARS");
}

static void compileReturn(const astReturnStatement* statement) {
	if (statement->hasValue) {
		compileExpression(&statement->value);
	}
	puts("RETURN");
}

static void compileBuiltInReadString() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@temp");
	puts("READ TF@temp string");
	puts("PUSHS TF@temp");
}

static void compileBuiltInReadInt() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@temp");
	puts("READ TF@temp int");
	puts("PUSHS TF@temp");
}

static void compileBuiltInReadDouble() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@temp");
	puts("READ TF@temp float");
	puts("PUSHS TF@temp");
}

static void compileBuiltInWrite(int parameterCount) {
	puts("CREATEFRAME");
	puts("DEFVAR TF@temp");
	for (int i = 0; i < parameterCount; i++) {
		puts("POPS TF@term");
		puts("WRITE TF@term");
	}
}

static void compileBuiltInInt2Double() { puts("INT2FLOATS"); }

static void compileBuiltInDouble2Int() { puts("DOUBLE2FLOATS"); }

static void compileBuiltInLength() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@temp");
	puts("POPS TF@temp");
	puts("STRLEN TF@temp TF@temp");
	puts("PUSHS TF@temp");
}

static void compileBuiltInSubstring() {
	// TODO
	assert(false);
}

static void compileBuiltInOrd() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@res");
	puts("DEFVAR TF@str");
	puts("POPS TF@str");
	puts("STR2INT TF@res TF@str 0");
	puts("PUSHS TF@res");
}

static void compileBuiltInChr() { puts("INT2CHARS"); }

static void compileInputParamList(const astInputParameterList* list) {
	// parameters are pushed to the stack right to left
	for (int i = list->count - 1; i >= 0; i--) {
		compileTerm(&(list->data[i].value));
	}
}

static void compileProcedureCall(const astProcedureCall* call) {
	compileInputParamList(&call->params);

	if (strcmp(call->procName.name, "write") == 0) {
		compileBuiltInWrite(call->params.count);
	} else {
		printf("CALL %s\n", call->procName.name);
	}

	puts("CLEARS");
}

static void compileFunctionCall(const astFunctionCall* call) {
	compileInputParamList(&call->params);

	// TODO - use a more effective way
	if (strcmp(call->funcName.name, "readString") == 0) {
		compileBuiltInReadString();
	} else if (strcmp(call->funcName.name, "readInt") == 0) {
		compileBuiltInReadInt();
	} else if (strcmp(call->funcName.name, "readDouble") == 0) {
		compileBuiltInReadDouble();
	} else if (strcmp(call->funcName.name, "Int2Double") == 0) {
		compileBuiltInInt2Double();
	} else if (strcmp(call->funcName.name, "Double2Int") == 0) {
		compileBuiltInDouble2Int();
	} else if (strcmp(call->funcName.name, "length") == 0) {
		compileBuiltInLength();
	} else if (strcmp(call->funcName.name, "substring") == 0) {
		compileBuiltInSubstring();
	} else if (strcmp(call->funcName.name, "ord") == 0) {
		compileBuiltInOrd();
	} else if (strcmp(call->funcName.name, "chr") == 0) {
		compileBuiltInChr();
	} else {
		printf("CALL %s\n", call->funcName.name);
	}

	printf("POPS ");
	emitVariableId(&call->varName);
	puts("");
	puts("CLEARS");
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
			compileConditional(&statement->conditional);
			break;
		case AST_STATEMENT_ITER:
			compileIteration(&statement->iteration);
			break;
		case AST_STATEMENT_FUNC_CALL:
			compileFunctionCall(&statement->functionCall);
			break;
		case AST_STATEMENT_PROC_CALL:
			compileProcedureCall(&statement->procedureCall);
			break;
		case AST_STATEMENT_RETURN:
			compileReturn(&statement->returnStmt);
			break;
	}
}

void compileFunctionDef(const astFunctionDefinition* def) {
	PUSH_FRAME();
	// parameters are read left to right
	for (int i = 0; i < def->params.count; i++) {
		printf("POPS LF@");
		emitVariableId(&(def->params.data[i].insideName));
		puts("");
	}

	compileStatementBlock(&def->body);
	POP_FRAME();
}

void compileProgram(const astProgram* program) {
	symStackCreate(&VAR_SYM_STACK);
	symStackPush(&VAR_SYM_STACK);  // global scope
	puts(".IFJcode23");
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_STATEMENT) {
			compileStatement(&topStatement->statement);
		} else {
			compileFunctionDef(&topStatement->functionDef);
		}
	}
}
