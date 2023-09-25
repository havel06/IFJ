#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int tokenCreate(token* newToken, tokenType type, const char* str, int strLength) {
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
