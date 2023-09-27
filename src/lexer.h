#ifndef LEXER_H
#define LEXER_H

typedef enum {
	TOKEN_EOF,
	TOKEN_IDENTIFIER,
	TOKEN_INT_LITERAL,
	TOKEN_DEC_LITERAL,
	TOKEN_STR_LITERAL,
	TOKEN_COMMA,
	TOKEN_ASSIGN,
	TOKEN_EQ,
	TOKEN_NEQ,
	TOKEN_LESS,
	TOKEN_GREATER,
	TOKEN_LESS_EQ,
	TOKEN_GREATER_EQ,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_UNWRAP,
	TOKEN_COALESCE,
	TOKEN_ARROW,
	TOKEN_BRACKET_ROUND_LEFT,
	TOKEN_BRACKET_ROUND_RIGHT,
	TOKEN_BRACKET_CURLY_LEFT,
	TOKEN_BRACKET_CURLY_RIGHT,
	TOKEN_COLON,
	TOKEN_UNDERSCORE,
	TOKEN_QUESTION_MARK,
	TOKEN_KEYWORD_DOUBLE,
	TOKEN_KEYWORD_ELSE,
	TOKEN_KEYWORD_FUNC,
	TOKEN_KEYWORD_IF,
	TOKEN_KEYWORD_INT,
	TOKEN_KEYWORD_LET,
	TOKEN_KEYWORD_NIL,
	TOKEN_KEYWORD_RETURN,
	TOKEN_KEYWORD_STRING,
	TOKEN_KEYWORD_VAR,
	TOKEN_KEYWORD_WHILE,
} tokenType;

typedef struct {
	tokenType type;
	char* content;
} token;

// Returns 0 on success
// int tokenCreate(token*, tokenType type, const char* str);
void tokenDestroy(token*);

// Returns 0 on success
typedef enum { LEXER_OK, LEXER_ERROR, LEXER_INTERNAL_ERROR } lexerResult;

lexerResult getNextToken(token*);
// dont't destroy token after returning!
// only return multiple tokens at once
void unGetToken(const token*);

#endif
