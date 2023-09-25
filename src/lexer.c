#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>

int tokenCreate(token* newToken, tokenType type, const char* str, int strLength) {
	newToken->type = type;

	if (!str && strLength == 0) {
		newToken->content = malloc(strLength * sizeof(char));
		if (!newToken) {
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
	// TODO
	return 0;
}

// Returns 0 if no whitespace was skipped
int skipWhiteSpace() {
	// TODO
	return 0;
}

int getNextToken(token *newToken) {
	while (skipWhiteSpace() || skipComments()) {
	}

	switch (getchar()) {
	case EOF:
		return tokenCreate(newToken, TOKEN_EOF, NULL, 0);
		// TODO
	}

	return 1;
}
