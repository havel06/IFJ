#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int tokenCreate(token* newToken, tokenType type, const char* str, int strLength) {
	assert(newToken);
	newToken->type = type;

	if (!str && strLength == 0) {
		newToken->content = malloc(strLength * sizeof(char));
		if (!(newToken->content)) {
			return 1;
		}
	} else {
		newToken->content = NULL;
	}

	return 0;
}

void tokenDestroy(token* tok) {
	assert(tok);
	if (tok->content) {
		free(tok->content);
	}
}

// Returns 0 if no comments were skipped
int skipComments() {
	// TODO - multiline comments
	int skipped = 0;
	while (1) {
		int slash1 = getchar();
		int slash2 = getchar();
		if (slash1 != '/' || slash2 != '/') {
			ungetc(slash2, stdin);
			ungetc(slash1, stdin);
			break;
		}

		// skip the comment
		while (1) {
			int c = getchar();
			if (c == '\n' || c == EOF) {
				break;
			} else {
				skipped++;
			}
		}
	}
	return skipped;
}

// Returns 0 if no whitespace was skipped
int skipWhiteSpace() {
	int skipped = 0;

	while (1) {
		int c = getchar();

		if (isspace(c)) {
			skipped++;
		} else {
			ungetc(c, stdin);
			break;
		}
	}

	return skipped;
}

int getNextToken(token* newToken) {
	assert(newToken);
	while (skipWhiteSpace() || skipComments()) {
	}

	switch (getchar()) {
		case EOF:
			return tokenCreate(newToken, TOKEN_EOF, NULL, 0);
		case '*':
			return tokenCreate(newToken, TOKEN_MUL, NULL, 0);
		case '/':
			return tokenCreate(newToken, TOKEN_DIV, NULL, 0);
		case '+':
			return tokenCreate(newToken, TOKEN_PLUS, NULL, 0);
		case ':':
			return tokenCreate(newToken, TOKEN_COLON, NULL, 0);
		case '!':
			return tokenCreate(newToken, TOKEN_UNWRAP, NULL, 0);
			// TODO
	}

	return 1;
}

void printToken(const token* tok) {
	assert(tok);
	switch (tok->type) {
		case TOKEN_EOF:
			puts("EOF");
			break;
		case TOKEN_QUOTE:
			puts("\"");
			break;
		case TOKEN_ASSIGN:
			puts("=");
			break;
		case TOKEN_EQ:
			puts("==");
			break;
		case TOKEN_NEQ:
			puts("!=");
			break;
		case TOKEN_LESS:
			puts("<");
			break;
		case TOKEN_GREATER:
			puts(">");
			break;
		case TOKEN_LESS_EQ:
			puts("<=");
			break;
		case TOKEN_GREATER_EQ:
			puts(">=");
			break;
		case TOKEN_MUL:
			puts("*");
			break;
		case TOKEN_DIV:
			puts("/");
			break;
		case TOKEN_PLUS:
			puts("+");
			break;
		case TOKEN_MINUS:
			puts("-");
			break;
		case TOKEN_UNWRAP:
			puts("!");
			break;
		case TOKEN_COALESCE:
			puts("??");
			break;
		case TOKEN_ARROW:
			puts("->");
			break;
		case TOKEN_BRACKET_ROUND_LEFT:
			puts("(");
			break;
		case TOKEN_BRACKET_ROUND_RIGHT:
			puts(")");
			break;
		case TOKEN_BRACKET_CURLY_LEFT:
			puts("{");
			break;
		case TOKEN_BRACKET_CURLY_RIGHT:
			puts("}");
			break;
		case TOKEN_COLON:
			puts(":");
			break;
		case TOKEN_UNDERSCORE:
			puts("_");
			break;
		case TOKEN_QUESTION_MARK:
			puts("?");
			break;
		case TOKEN_KEYWORD_DOUBLE:
			puts("Double");
			break;
		case TOKEN_KEYWORD_ELSE:
			puts("else");
			break;
		case TOKEN_KEYWORD_FUNC:
			puts("func");
			break;
		case TOKEN_KEYWORD_IF:
			puts("if");
			break;
		case TOKEN_KEYWORD_INT:
			puts("Int");
			break;
		case TOKEN_KEYWORD_LET:
			puts("let");
			break;
		case TOKEN_KEYWORD_NIL:
			puts("nil");
			break;
		case TOKEN_KEYWORD_RETURN:
			puts("return");
			break;
		case TOKEN_KEYWORD_STRING:
			puts("String");
			break;
		case TOKEN_KEYWORD_VAR:
			puts("var");
			break;
		case TOKEN_KEYWORD_WHILE:
			puts("while");
			break;
		case TOKEN_IDENTIFIER:
			printf("IDENTIFIER: %s\n", tok->content);
			break;
		case TOKEN_INT_LITERAL:
			printf("INTEGER LITERAL: %s\n", tok->content);
			break;
		case TOKEN_DEC_LITERAL:
			printf("DECIMAL LITERAL: %s\n", tok->content);
			break;
		case TOKEN_STR_LITERAL:
			printf("STRING LITERAL: %s\n", tok->content);
			break;
	}
}
