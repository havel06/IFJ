#ifndef LEXER_H
#define LEXER_H

typedef enum {
	TOKEN_EOF
	// TODO
} tokenType;

typedef struct {
	tokenType type;
	char* content;
} token;

// Returns 0 on success
int tokenCreate(token*, tokenType type, const char* str, int strLength);
void tokenDestroy(token*);

// Returns 0 on success
int getNextToken(token*);
void printToken(const token*);

#endif
