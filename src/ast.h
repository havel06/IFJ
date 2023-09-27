#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef struct astStatement astStatement;	 // fwd
typedef struct astExpression astExpression;	 // fwd

typedef struct {
	char* name;
} astIdentifier;

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
	AST_TERM_NIL,
} astTermType;

typedef struct {
	astTermType type;
	union {
		astIdentifier identifier;
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

typedef struct {
	astBinaryOperator op;
	astExpression* lhs;
	astExpression* rhs;
} astBinaryExpression;

typedef struct {
	astExpression* innerExpr;
} astUnwrapExpression;

typedef enum { AST_EXPR_TERM, AST_EXPR_BINARY, AST_EXPR_UNWRAP } astExpressionType;

struct astExpression {
	astExpressionType type;
	union {
		astTerm term;
		astBinaryExpression binary;
		astUnwrapExpression unwrap;
	};
};

typedef enum {
	AST_STATEMENT_VAR_DEF,
	AST_STATEMENT_ASSIGN,
	AST_STATEMENT_COND,
	AST_STATEMENT_ITER,
	AST_STATEMENT_FUNC_CALL,
	AST_STATEMENT_PROC_CALL,
	AST_STATEMENT_RETURN,
} astStatementType;

typedef enum {
	AST_TYPE_UNSPECIFIED,
	AST_TYPE_INT,
	AST_TYPE_DOUBLE,
	AST_TYPE_STRING,
} astBasicDataType;

typedef struct {
	astBasicDataType type;
	bool nullable;
} astDataType;

typedef struct {
	char* variableName;	 // TODO - use identifier
	astDataType variableType;
	astExpression value;
	bool immutable;
} astVariableDefinition;

typedef struct {
	char* variableName;	 // TODO - use identifier
	astExpression value;
} astAssignment;

typedef struct {
	astStatement* statements;
	int count;
} astStatementBlock;

typedef struct {
	astIdentifier identifier;
} astOptionalBinding;

typedef enum {
	AST_CONDITION_OPT_BINDING,
	AST_CONDITION_EXPRESSION,
} astConditionType;

typedef struct {
	astConditionType type;
	union {
		astExpression expression;
		astOptionalBinding optBinding;
	};
} astCondition;

typedef struct {
	astCondition condition;
	astStatementBlock body;
	astStatementBlock bodyElse;
	bool hasElse;
} astConditional;

typedef struct {
	astExpression condition;
	astStatementBlock body;
} astIteration;

typedef struct {
	astIdentifier name;
	astTerm value;
} astInputParameter;

typedef struct {
	astInputParameter* data;
	int count;
} astInputParameterList;

typedef struct {
	astIdentifier varName;
	astIdentifier funcName;
	astInputParameterList params;
} astFunctionCall;

typedef struct {
	// TODO
} astProcedureCall;

typedef struct {
	bool has_value;
	astExpression value;
} astReturnStatement;

struct astStatement {
	astStatementType type;
	union {
		astVariableDefinition variableDef;
		astAssignment assignment;
		astConditional conditional;
		astIteration iteration;
		astFunctionCall functionCall;
		astProcedureCall procedureCall;
		astReturnStatement returnStmt;
	};
};

typedef struct {
	bool requiresName;
	astIdentifier outsideName;
	astIdentifier insideName;
	astDataType dataType;
} astParameter;

typedef struct {
	astParameter* data;
	int count;
} astParameterList;

typedef struct {
	astIdentifier name;
	astParameterList params;
	bool hasReturnValue;
	astDataType returnType;
	astStatementBlock body;
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
int astBinaryExprCreate(astExpression*, astExpression lhs, astExpression rhs, astBinaryOperator);
void astBinaryExprDestroy(astExpression*);

// Returns 0 on success
int astIdentCreate(astIdentifier*, const char* str);
void astIdentDestroy(astIdentifier*);

// Returns 0 on success
int astVarDefCreate(astVariableDefinition*, const char* str, astDataType, astExpression, bool immutable);
void astVarDefDestroy(astVariableDefinition*);

// Returns 0 on success
int astStringCreate(astStringLiteral*, const char* str);
void astStringDestroy(astStringLiteral*);

void astStatementBlockCreate(astStatementBlock*);
// Returns 0 on success
int astStatementBlockAdd(astStatementBlock*, astStatement);
void astStatementBlockDestroy(astStatementBlock*);

// void astParameterDestroy();

void astParameterListCreate(astParameterList*);
// Returns 0 on success
int astParameterListAdd(astParameterList*, astParameter);
// void astParameterListDestroy();
//
void astInputParameterListCreate(astInputParameterList*);
int astInputParameterListAdd(astInputParameterList*, astInputParameter);

#endif
