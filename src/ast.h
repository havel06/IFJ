#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef struct {
	char* name;
} astIdentificator;

typedef struct {
	int value;
} astIntLiteral;

typedef struct {
	double value;
} astDecimalLiteral;

typedef struct {
	char* content;
} astStringLiteral;

typedef enum {
	AST_TERM_ID,
	AST_TERM_INT,
	AST_TERM_DECIMAL,
	AST_TERM_STRING,
} astTermType;

typedef struct {
	astTermType type;
	union {
		astIdentificator identificator;
		astIntLiteral integer;
		astDecimalLiteral decimal;
		astStringLiteral string;
	};
} astTerm;

typedef enum {
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

typedef struct astExpression astExpression;	 // fwd

typedef struct {
	astBinaryOperator op;
	astExpression* lhs;
	astExpression* rhs;
} astBinaryExpression;

typedef enum { AST_EXPR_TERM, AST_EXPR_BINARY, AST_EXPR_UNWRAP } astExpressionType;

struct astExpression {
	astExpressionType type;
	union {
		astTerm term;
		astBinaryExpression binary;
	};
};

typedef enum {
	AST_STATEMENT_VAR_DEF,
	AST_STATEMENT_ASSIGN,
	AST_STATEMENT_COND,
	AST_STATEMENT_ITER,
	AST_STATEMENT_FUNC_CALL,
	AST_STATEMENT_FUNC_CALL_VOID,
	AST_STATEMENT_RETURN,
} astStatementType;

typedef enum {
	AST_TYPE_UNSPECIFIED,
	AST_TYPE_INT,
	AST_TYPE_DOUBLE,
	AST_TYPE_STRING,
} astDataType;

typedef struct {
	char* variableName;
	astDataType variableType;
	astExpression value;
	bool immutable;
} astVariableDefinition;

typedef struct {
	// TODO
} astAssignment;

typedef struct {
	// TODO
} astConditional;

typedef struct {
	// TODO
} astIteration;

typedef struct {
	// TODO
} astFunctionCall;

typedef struct {
	// TODO
} astVoidFunctionCall;

typedef struct {
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

typedef struct {
	// TODO
} astFunctionDefinition;

typedef enum { AST_TOP_FUNCTION, AST_TOP_STATEMENT } astTopLevelStatementType;

typedef struct {
	astTopLevelStatementType type;
	union {
		astStatement statement;
		astFunctionDefinition functionDef;
	};
} astTopLevelStatement;

typedef struct {
	astTopLevelStatement* statements;
	int count;
} astProgram;

void astProgramCreate(astProgram*);
void astProgramDestroy(astProgram*);
// Returns 0 on success
int astProgramAdd(astProgram*, astTopLevelStatement);

// Returns 0 on success
int astBinaryExprCreate(astBinaryExpression*, astExpression lhs, astExpression rhs, astBinaryOperator);
void astBinaryExprDestroy(astBinaryExpression*);

// Returns 0 on success
int astIdentCreate(astIdentificator*, const char* str);
void astIdentDestroy(astIdentificator*);

// Returns 0 on success
int astVarDefCreate(astVariableDefinition*, const char* str, astDataType, astExpression, bool immutable);
void astVarDefDestroy(astVariableDefinition*);

// Returns 0 on success
int astStringCreate(astStringLiteral*, const char* str);
void astStringDestroy(astStringLiteral*);

void astPrint(const astProgram*);

#endif
