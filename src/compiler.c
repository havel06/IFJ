#include "compiler.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "symtable.h"

static symbolTableStack VAR_SYM_STACK;
static const symbolTable* FUNC_SYM_TABLE;

#define PUSH_FRAME()     \
	puts("CREATEFRAME"); \
	puts("PUSHFRAME");   \
	symStackPush(&VAR_SYM_STACK);

#define POP_FRAME()   \
	puts("POPFRAME"); \
	symStackPop(&VAR_SYM_STACK);

// used for compiler-generated labes (in conditionals etc)
static int LAST_LABEL_NAME = 0;

static int newLabelName() { return LAST_LABEL_NAME++; }

// forward decl
static astDataType compileExpression(const astExpression*);
static void compileStatement(const astStatement*);

static void emitNewVariableId(const astIdentifier* var) {
	symbolTable* currentScope = symStackCurrentScope(&VAR_SYM_STACK);
	if (currentScope == symStackGlobalScope(&VAR_SYM_STACK)) {
		printf("GF@%s", var->name);
	} else {
		printf("LF@v%d%s", currentScope->id, var->name);
	}
}

static void emitVariableId(const astIdentifier* var) {
	symbolTable* scope;
	symStackLookup(&VAR_SYM_STACK, var->name, &scope);
	if (scope == symStackGlobalScope(&VAR_SYM_STACK)) {
		printf("GF@%s", var->name);
	} else {
		printf("LF@v%d%s", scope->id, var->name);
	}
}

static astDataType compileTerm(const astTerm* term) {
	astDataType dataType;
	dataType.nullable = false;

	switch (term->type) {
		case AST_TERM_ID: {
			printf("PUSHS ");
			emitVariableId(&term->identifier);
			puts("");
			symbolTableSlot* slot = symStackLookup(&VAR_SYM_STACK, term->identifier.name, NULL);
			dataType = slot->variable.type;
			break;
		}
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
			printf("PUSHS string@");
			for (size_t i = 0; i < strlen(term->string.content); i++) {
				char c = term->string.content[i];
				if (isspace(c) || !isprint(c) || c == '#' || c == '\\') {
					printf("\\%03d", (int)c);
				} else {
					putchar(c);
				}
			}
			puts("");
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
	puts("DEFVAR TF@res");
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
			printf("JUMPIFNEQ l%d TF@res nil@nil\n", coalLabel);
			puts("MOVE TF@res TF@rhs");
			printf("LABEL l%d\n", coalLabel);
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

static void compileStatementBlock(const astStatementBlock* block) {
	symStackPush(&VAR_SYM_STACK);
	for (int i = 0; i < block->count; i++) {
		compileStatement(&block->statements[i]);
	}
	symStackPop(&VAR_SYM_STACK);
}

static void compileOptionalBinding(const astOptionalBinding* binding) {
	printf("PUSHS ");
	emitVariableId(&binding->identifier);
	puts("");
	puts("PUSHS nil@nil");
	puts("EQS");
	puts("NOTS");
}

static void compileConditional(const astConditional* conditional) {
	astConditionType conditionType = conditional->condition.type;
	if (conditionType == AST_CONDITION_EXPRESSION) {
		compileExpression(&conditional->condition.expression);
	} else {
		compileOptionalBinding(&conditional->condition.optBinding);
	}

	int label1 = newLabelName();
	puts("PUSHS bool@true");
	printf("JUMPIFNEQS l%d\n", label1);

	compileStatementBlock(&conditional->body);

	if (!conditional->hasElse) {
		printf("LABEL l%d\n", label1);
		puts("CLEARS");
		return;
	}

	int label2 = newLabelName();
	printf("JUMP l%d\n", label2);
	printf("LABEL l%d\n", label1);

	compileStatementBlock(&conditional->bodyElse);

	printf("LABEL l%d\n", label2);
	puts("CLEARS");
}

static void compileIteration(const astIteration* iteration) {
	int startLabel = newLabelName();
	int condLabel = newLabelName();
	printf("JUMP l%d\n", condLabel);
	printf("LABEL l%d\n", startLabel);
	// body
	compileStatementBlock(&iteration->body);
	// condition
	printf("LABEL l%d\n", condLabel);
	compileExpression(&iteration->condition);
	puts("PUSHS bool@true");
	printf("JUMPIFEQS l%d\n", startLabel);
	puts("CLEARS");
}

static void compileReturn(const astReturnStatement* statement) {
	if (statement->hasValue) {
		compileExpression(&statement->value);
	}
	puts("POPFRAME");
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
		puts("POPS TF@temp");
		puts("WRITE TF@temp");
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
		printf("CALL l%s\n", call->procName.name);
	}

	puts("CLEARS");
}

static void compileFunctionCall(const astFunctionCall* call, bool newVariable) {
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
		printf("CALL l%s\n", call->funcName.name);
	}

	printf("POPS ");

	if (newVariable) {
		emitNewVariableId(&call->varName);
	} else {
		emitVariableId(&call->varName);
	}

	puts("");
	puts("CLEARS");
}

static void compileVariableDef(const astVariableDefinition* def) {
	printf("DEFVAR ");
	emitNewVariableId(&def->variableName);
	puts("");

	astDataType variableType = def->variableType;
	if (def->hasInitValue) {
		if (def->value.type == AST_VAR_INIT_EXPR) {
			variableType = compileExpression(&def->value.expr);
			printf("POPS ");
			emitNewVariableId(&def->variableName);
			puts("");
		} else {
			const symbolTableSlot* funcSlot =
				symTableLookup((symbolTable*)FUNC_SYM_TABLE, def->value.call.funcName.name);
			assert(funcSlot);
			variableType = funcSlot->function.returnType;
			compileFunctionCall(&def->value.call, true);
		}
	}

	// insert into symtable
	symbolVariable newVar = {variableType, def->immutable, NULL};
	symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), newVar, def->variableName.name);
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
			compileFunctionCall(&statement->functionCall, false);
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
	int funcEndLabel = newLabelName();
	printf("JUMP l%d\n", funcEndLabel);
	printf("LABEL l%s\n", def->name.name);
	PUSH_FRAME();
	// add params to symtable
	for (int i = 0; i < def->params.count; i++) {
		astParameter* param = &def->params.data[i];
		symbolVariable symbol = {param->dataType, true, symStackCurrentScope(&VAR_SYM_STACK)};
		symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), symbol, param->insideName.name);
	}
	symStackPush(&VAR_SYM_STACK);  // create new symtable scope so variables can shadow params
	// parameters are read left to right
	for (int i = 0; i < def->params.count; i++) {
		astParameter* param = &def->params.data[i];
		printf("DEFVAR ");
		emitVariableId(&(param->insideName));
		puts("");
		printf("POPS ");
		emitVariableId(&(param->insideName));
		puts("");
	}
	compileStatementBlock(&def->body);
	symStackPop(&VAR_SYM_STACK);
	POP_FRAME();
	puts("RETURN");
	printf("LABEL l%d\n", funcEndLabel);
}

void compileProgram(const astProgram* program, const symbolTable* functionTable) {
	FUNC_SYM_TABLE = functionTable;
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
