#ifndef LEXER_H
#define LEXER_H

typedef enum {
	// TODO
} tokenType;

typedef struct {
	tokenType type;
	char* content;
} token;

int tokenCreate(tokenType type, const char* str, int strLength);

token getNextToken();

#endif
