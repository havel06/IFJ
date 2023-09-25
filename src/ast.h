#ifndef AST_H
#define AST_H

typedef enum
{
	AST_STATEMENT_VAR_DEF,
	AST_STATEMENT_ASSIGN,
	AST_STATEMENT_COND,
	AST_STATEMENT_ITER,
	AST_STATEMENT_FUNC_CALL,
	AST_STATEMENT_FUNC_CALL_VOID,
	AST_STATEMENT_RETURN,
} astStatementType;

typedef struct 
{
	//TODO
} astVariableDefinition ;

typedef struct
{
	//TODO
} astAssignment;

typedef struct
{
	//TODO
} astConditional;

typedef struct
{
	//TODO
} astIteration;

typedef struct
{
	//TODO
} astFunctionCall;

typedef struct
{
	//TODO
} astVoidFunctionCall;

typedef struct
{
	astStatementType type;
	union {
		astVariableDefinition variableDef;
		astAssignment assignment;
		astConditional conditional;
		astIteration iteration;
		astFunctionCall functionCall;
		astVoidFunctionCall voidFunctionCall;
	};
} astStatement;

typedef struct
{
	//TODO
} astFunctionDefinition;

typedef enum
{
	AST_TOP_FUNCTION,
	AST_TOP_STATEMENT
} astTopLevelStatementType;

typedef struct
{
	astTopLevelStatementType type;
	union
	{
		astStatement statement;
		astFunctionDefinition functionDef;
	};
} astTopLevelStatement;

typedef struct
{
	astTopLevelStatement* statements;
} astProgram;

#endif
