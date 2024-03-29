/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

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

void tokenDestroy(token*);

typedef enum { LEXER_OK, LEXER_ERROR, LEXER_INTERNAL_ERROR } lexerResult;

// returns next token in stream
lexerResult getNextToken(token*);

// Returns token to internal buffer.
// dont't destroy token after returning!
// don't return a token when there is something in buffer
void unGetToken(const token*);

#endif
