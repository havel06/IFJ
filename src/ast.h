#ifndef AST_H
#define AST_H

typedef struct
{
	char* name;
} astIdentificator;

typedef struct
{
	int value;
} astIntLiteral;

typedef struct
{
	double value;
} astDecimalLiteral;

typedef struct
{
	char* content;
} astStringLiteral;

typedef enum
{
	AST_TERM_ID,
	AST_TERM_INT,
	AST_TERM_DECIMAL,
	AST_TERM_STRING,
} astTermType;

typedef struct
{
	astTermType type;
	union
	{
		astIdentificator identificator;
		astIntLiteral integer;
		astDecimalLiteral decimal;
		astStringLiteral string;
	};
} astTerm;

typedef enum
{
	AST_BINARY_MUL,
	AST_BINARY_DIV,
	AST_BINARY_PLUS,
	AST_BINARY_MINUS,
	AST_BINARY_EQ,
	AST_BINARY_NEQ,
	AST_BINARY_LESS,
	AST_BINARY_GREATER,
	AST_BINARY_LESS_EQ,
	AST_BINARY_GREATER_EQ,
	AST_BINARY_NIL_COAL
} astBinaryOperator;

typedef struct astExpression astExpression; // fwd

typedef struct
{
	astBinaryOperator op;
	astExpression* lhs;
	astExpression* rhs;
} astBinaryExpression;

typedef enum
{
	AST_EXPR_TERM,
	AST_EXPR_BINARY,
	AST_EXPR_UNWRAP
} astExpressionType;

struct astExpression
{
	astExpressionType type;
	union
	{
		astTerm term;
		astBinaryExpression binary;
	};
};

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
	int count;
} astProgram;

int astProgramCreate(astProgram*);
void astProgramDestroy(astProgram*);
int astProgramAdd(astTopLevelStatement);

int astBinaryExprCreate(astBinaryExpression*, astExpression lhs, astExpression rhs, astBinaryOperator);
void astBinaryExprDestroy(astBinaryExpression*);

int astIdentCreate(astIdentificator*, const char* str, int length);
void astIdentDestroy(astIdentificator*);

int astStringCreate(astStringLiteral*, const char* str, int length);
void astStringDestroy(astStringLiteral*);

void astPrint(const astProgram*);

#endif
