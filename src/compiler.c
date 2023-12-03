/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

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
static void compileStatement(const astStatement*, bool noDeclareVars);
static void compileVariableDef(const astVariableDefinition* def, bool assignmentOnly, bool predefine);

static void emitNewVariableId(const astIdentifier* var) {
	symbolTable* currentScope = symStackCurrentScope(&VAR_SYM_STACK);
	if (currentScope == symStackGlobalScope(&VAR_SYM_STACK)) {
		printf("GF@%s", var->name);
	} else {
		printf("LF@v%d%s", currentScope->id, var->name);
	}
}

static void emitVariableId(const astIdentifier* var) {
	symbolTable* scope = NULL;
	symStackLookup(&VAR_SYM_STACK, var->name, &scope);
	assert(scope);
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
					printf("\\%03d", (int)*((unsigned char*)(&c)));
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

	if (expr->op != AST_BINARY_NIL_COAL) {
		// perform conversion
		if (lhsType.type == rhsType.type) {
			// do nothing
		} else if (lhsType.nullable && rhsType.type == AST_TYPE_NIL) {
			// do nothing (for equality comparisons)
		} else if (rhsType.nullable && lhsType.type == AST_TYPE_NIL) {
			// do nothing (for equality comparisons)
		} else if (lhsType.type == AST_TYPE_INT) {
			puts("INT2FLOAT TF@lhs TF@lhs");
			lhsType.type = AST_TYPE_DOUBLE;
		} else if (rhsType.type == AST_TYPE_INT) {
			puts("INT2FLOAT TF@rhs TF@rhs");
			rhsType.type = AST_TYPE_DOUBLE;
		}
	}

	switch (expr->op) {
		case AST_BINARY_MUL:
			puts("MUL TF@res TF@lhs TF@rhs");
			resultType.type = lhsType.type;
			break;
		case AST_BINARY_DIV:
			if (lhsType.type == AST_TYPE_INT) {
				puts("IDIV TF@res TF@lhs TF@rhs");
			} else {
				puts("DIV TF@res TF@lhs TF@rhs");
			}
			resultType.type = lhsType.type;
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
			resultType.type = AST_TYPE_BOOL;
			break;
		case AST_BINARY_NIL_COAL: {
			int coalLabel = newLabelName();
			puts("MOVE TF@res TF@lhs");
			printf("JUMPIFNEQ l%d TF@res nil@nil\n", coalLabel);
			puts("MOVE TF@res TF@rhs");
			printf("LABEL l%d\n", coalLabel);
			resultType.type = rhsType.type;
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

static void compileStatementBlock(const astStatementBlock* block, bool noDeclareVars) {
	symStackPush(&VAR_SYM_STACK);
	for (int i = 0; i < block->count; i++) {
		compileStatement(&block->statements[i], noDeclareVars);
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

static void compileConditional(const astConditional* conditional, bool noDeclareVars) {
	astConditionType conditionType = conditional->condition.type;
	if (conditionType == AST_CONDITION_EXPRESSION) {
		compileExpression(&conditional->condition.expression);
	} else {
		compileOptionalBinding(&conditional->condition.optBinding);
	}

	int label1 = newLabelName();
	puts("PUSHS bool@true");
	printf("JUMPIFNEQS l%d\n", label1);

	compileStatementBlock(&conditional->body, noDeclareVars);

	if (!conditional->hasElse) {
		printf("LABEL l%d\n", label1);
		puts("CLEARS");
		return;
	}

	int label2 = newLabelName();
	printf("JUMP l%d\n", label2);
	printf("LABEL l%d\n", label1);

	compileStatementBlock(&conditional->bodyElse, noDeclareVars);

	printf("LABEL l%d\n", label2);
	puts("CLEARS");
}

static void precompileVariableDefs(const astStatementBlock* block) {
	for (int i = 0; i < block->count; i++) {
		const astStatement* statement = &block->statements[i];
		switch (statement->type) {
			case AST_STATEMENT_VAR_DEF: {
				compileVariableDef(&statement->variableDef, false, true);
				break;
			}

			case AST_STATEMENT_COND:
				precompileVariableDefs(&statement->conditional.body);
				if (statement->conditional.hasElse) {
					precompileVariableDefs(&statement->conditional.bodyElse);
				}
				break;

			case AST_STATEMENT_ITER:
				precompileVariableDefs(&statement->iteration.body);
				break;

			default:
				// nothing
				break;
		}
	}
}

static void compileIteration(const astIteration* iteration, bool noDeclareVars) {
	symStackPush(&VAR_SYM_STACK);  // used for predefined variables
	if (!noDeclareVars) {
		precompileVariableDefs(&iteration->body);
	}
	int startLabel = newLabelName();
	int condLabel = newLabelName();
	printf("JUMP l%d\n", condLabel);
	printf("LABEL l%d\n", startLabel);
	// body
	compileStatementBlock(&iteration->body, true);
	// condition
	symStackPop(&VAR_SYM_STACK);  // used for predefined variables
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

static void compileBuiltInDouble2Int() { puts("FLOAT2INTS"); }

static void compileBuiltInLength() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@temp");
	puts("POPS TF@temp");
	puts("STRLEN TF@temp TF@temp");
	puts("PUSHS TF@temp");
}

static void compileBuiltInSubstring() {
	puts("CREATEFRAME");
	// parameters
	puts("DEFVAR TF@str");
	puts("POPS TF@str");
	puts("DEFVAR TF@start");
	puts("POPS TF@start");
	puts("DEFVAR TF@end");
	puts("POPS TF@end");

	// parameter errors
	int errorLabel = newLabelName();
	puts("DEFVAR TF@error_temp");
	puts("DEFVAR TF@len");
	puts("STRLEN TF@len TF@str");
	// start < 0
	puts("LT TF@error_temp TF@start int@0");
	printf("JUMPIFEQ l%d TF@error_temp bool@true\n", errorLabel);
	// end < 0
	puts("LT TF@error_temp TF@end int@0");
	printf("JUMPIFEQ l%d TF@error_temp bool@true\n", errorLabel);
	// start > end
	puts("GT TF@error_temp TF@start TF@end");
	printf("JUMPIFEQ l%d TF@error_temp bool@true\n", errorLabel);
	// start >= length
	puts("LT TF@error_temp TF@start TF@len");  // !(start < length)
	printf("JUMPIFEQ l%d TF@error_temp bool@false\n", errorLabel);

	// create string
	puts("DEFVAR TF@result");
	puts("MOVE TF@result string@");
	puts("DEFVAR TF@index");
	puts("MOVE TF@index TF@start");
	puts("DEFVAR TF@char");
	// loop
	int loop0 = newLabelName();
	int loop1 = newLabelName();
	printf("JUMP l%d\n", loop0);
	printf("LABEL l%d\n", loop1);
	puts("GETCHAR TF@char TF@str TF@index");
	puts("CONCAT TF@result TF@result TF@char");
	puts("ADD TF@index TF@index int@1");
	printf("LABEL l%d\n", loop0);
	printf("JUMPIFNEQ l%d TF@index TF@end\n", loop1);

	// return value
	puts("PUSHS TF@result");
	int noErrorLabel = newLabelName();
	printf("JUMP l%d\n", noErrorLabel);

	// return nil on error
	printf("LABEL l%d\n", errorLabel);
	puts("PUSHS nil@nil");

	printf("LABEL l%d\n", noErrorLabel);
}

static void compileBuiltInOrd() {
	puts("CREATEFRAME");
	puts("DEFVAR TF@res");
	puts("DEFVAR TF@str");
	puts("POPS TF@str");
	puts("DEFVAR TF@len");
	puts("STRLEN TF@len TF@str");

	int errorLabel = newLabelName();
	int endLabel = newLabelName();

	// error check
	printf("JUMPIFEQ l%d TF@len int@0\n", errorLabel);

	// normal path
	puts("STRI2INT TF@res TF@str int@0");
	printf("JUMP l%d\n", endLabel);

	// error path
	printf("LABEL l%d\n", errorLabel);
	puts("MOVE TF@res int@0");

	// common path
	printf("LABEL l%d\n", endLabel);
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

static void compileVariableDef(const astVariableDefinition* def, bool assignmentOnly, bool predefine) {
	if (!assignmentOnly) {
		printf("DEFVAR ");
		emitNewVariableId(&def->variableName);
		puts("");
	} else {
		// assignment only - validate in symtable
		symStackValidate(&VAR_SYM_STACK, def->variableName.name);
	}

	astDataType variableType = def->variableType;
	if (def->hasInitValue) {
		if (!predefine) {
			if (def->value.type == AST_VAR_INIT_EXPR) {
				// compile initialiser
				astDataType expressionType = compileExpression(&def->value.expr);
				if (!def->hasExplicitType) {
					if (assignmentOnly) {
						symStackSetVarType(&VAR_SYM_STACK, def->variableName.name, expressionType);
					} else {
						variableType = expressionType;
					}
				}
				// convert int to double if needed
				if (def->hasExplicitType && def->variableType.type == AST_TYPE_DOUBLE &&
					expressionType.type == AST_TYPE_INT) {
					puts("INT2FLOATS");
				}
				printf("POPS ");
				if (assignmentOnly) {
					emitVariableId(&def->variableName);
				} else {
					emitNewVariableId(&def->variableName);
				}
				puts("");
			} else {
				// copmpile initialiser
				const symbolTableSlot* funcSlot =
					symTableLookup((symbolTable*)FUNC_SYM_TABLE, def->value.call.funcName.name);
				assert(funcSlot);
				if (assignmentOnly) {
					symStackSetVarType(&VAR_SYM_STACK, def->variableName.name, funcSlot->function.returnType);
				} else {
					variableType = funcSlot->function.returnType;
				}
				compileFunctionCall(&def->value.call, !assignmentOnly);
			}
		}
	} else if (def->variableType.nullable) {
		// default nil init
		printf("MOVE ");
		emitNewVariableId(&def->variableName);
		puts(" nil@nil");
	}

	if (!assignmentOnly) {
		// insert into symtable
		symbolVariable newVar = {variableType, def->immutable, NULL};
		assert(symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), newVar, def->variableName.name, !predefine));
	}
}

static void compileStatement(const astStatement* statement, bool noDeclareVars) {
	switch (statement->type) {
		case AST_STATEMENT_VAR_DEF:
			compileVariableDef(&statement->variableDef, noDeclareVars, false);
			break;
		case AST_STATEMENT_ASSIGN:
			compileAssignment(&statement->assignment);
			break;
		case AST_STATEMENT_COND:
			compileConditional(&statement->conditional, noDeclareVars);
			break;
		case AST_STATEMENT_ITER:
			compileIteration(&statement->iteration, noDeclareVars);
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
		assert(symTableInsertVar(symStackCurrentScope(&VAR_SYM_STACK), symbol, param->insideName.name, true));
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
	compileStatementBlock(&def->body, false);
	symStackPop(&VAR_SYM_STACK);
	POP_FRAME();
	puts("RETURN");
	printf("LABEL l%d\n", funcEndLabel);
}

void compileProgram(const astProgram* program, const symbolTable* functionTable) {
	FUNC_SYM_TABLE = functionTable;
	symStackCreate(&VAR_SYM_STACK);
	puts(".IFJcode23");
	PUSH_FRAME();  // global scope + frame for local variables that are not in functions
	for (int i = 0; i < program->count; i++) {
		const astTopLevelStatement* topStatement = &program->statements[i];
		if (topStatement->type == AST_TOP_STATEMENT) {
			compileStatement(&topStatement->statement, false);
		} else {
			compileFunctionDef(&topStatement->functionDef);
		}
	}
	POP_FRAME();
}
