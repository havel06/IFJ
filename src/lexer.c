#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static token BUFFERED_TOKEN;
static bool BUFFERED_TOKEN_EMPTY = true;

void unGetToken(const token* tok) {
	assert(BUFFERED_TOKEN_EMPTY);
	BUFFERED_TOKEN = *tok;
	BUFFERED_TOKEN_EMPTY = false;
}

/*
int tokenCreate(token* newToken, tokenType type, const char* str) {
	assert(newToken);
	newToken->type = type;

	if (str) {
		newToken->content = malloc(strlen(str) * sizeof(char) + 1);
		if (!(newToken->content)) {
			return 1;
		}
	} else {
		newToken->content = NULL;
	}

	return 0;
}
*/

void tokenDestroy(token* tok) {
	assert(tok);
	if (tok->content) {
		free(tok->content);
		tok->content = NULL;
	}
}

// Returns 0 if no comments were skipped
int skipComments() {
	// TODO - multiline comments
	int skipped = 0;
	while (1) {
		int slash1 = getchar();
		int slash2 = getchar();
		// single line comment
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
	// mulitline comment - possible bugs
	while (1) {
		int slash = getchar();
		int asterisk = getchar();
		if (slash != '/' || asterisk != '*') {
			ungetc(asterisk, stdin);
			ungetc(slash, stdin);
			break;
		}
		int nextC = getchar();
		while (1) {
			int c = nextC;
			nextC = getchar();
			if (c == '*' || nextC == '/') {
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

void checkForKeyword(token* newToken) {
	assert(newToken);
	assert(newToken->content);

	if (strcmp(newToken->content, "Double") == 0) {
		newToken->type = TOKEN_KEYWORD_DOUBLE;
	} else if (strcmp(newToken->content, "else") == 0) {
		newToken->type = TOKEN_KEYWORD_ELSE;
	} else if (strcmp(newToken->content, "func") == 0) {
		newToken->type = TOKEN_KEYWORD_FUNC;
	} else if (strcmp(newToken->content, "if") == 0) {
		newToken->type = TOKEN_KEYWORD_IF;
	} else if (strcmp(newToken->content, "Int") == 0) {
		newToken->type = TOKEN_KEYWORD_INT;
	} else if (strcmp(newToken->content, "let") == 0) {
		newToken->type = TOKEN_KEYWORD_LET;
	} else if (strcmp(newToken->content, "nil") == 0) {
		newToken->type = TOKEN_KEYWORD_NIL;
	} else if (strcmp(newToken->content, "return") == 0) {
		newToken->type = TOKEN_KEYWORD_RETURN;
	} else if (strcmp(newToken->content, "String") == 0) {
		newToken->type = TOKEN_KEYWORD_STRING;
	} else if (strcmp(newToken->content, "var") == 0) {
		newToken->type = TOKEN_KEYWORD_VAR;
	} else if (strcmp(newToken->content, "while") == 0) {
		newToken->type = TOKEN_KEYWORD_WHILE;
	}
}

lexerResult lexIdentifierToken(token* newToken) {
	assert(newToken);
	newToken->type = TOKEN_IDENTIFIER;
	while (1) {
		int c = getchar();
		if (!isalnum(c) && c != '_') {
			ungetc(c, stdin);
			break;
		}

		// add char to token
		if (newToken->content == NULL) {
			newToken->content = calloc(64, sizeof(char));
			if (newToken->content == NULL) {
				return LEXER_INTERNAL_ERROR;
			}
			newToken->content[0] = c;
		} else {
			int len = strlen(newToken->content);
			if (len >= 63) {
				// TODO - log warning
				return LEXER_INTERNAL_ERROR;
			}
			newToken->content[len] = c;
		}
	}

	checkForKeyword(newToken);
	return LEXER_OK;
}

lexerResult lexNumberToken(token* newToken) {
	assert(newToken);

	enum { INT_PART, DEC_PART, EXP_PART } numberPart;
	numberPart = INT_PART;
	newToken->type = TOKEN_INT_LITERAL;

	while (1) {
		int c = getchar();

		switch (c) {
			case '.':
				if (numberPart != INT_PART) {
					ungetc(c, stdin);
					printf("dot\n");
					return LEXER_ERROR;
				}
				numberPart = DEC_PART;
				newToken->type = TOKEN_DEC_LITERAL;
				break;
			case 'e':
			case 'E':
				if (numberPart == EXP_PART) {
					ungetc(c, stdin);
					return LEXER_ERROR;
				}
				numberPart = EXP_PART;
				break;
			default:
				if (!isdigit(c)) {
					ungetc(c, stdin);
					return 0;
				}
		}

		// init token content if empty
		if (newToken->content == NULL) {
			newToken->content = calloc(1024, sizeof(char));
			if (newToken->content == NULL) {
				return LEXER_INTERNAL_ERROR;
			}
		}

		// add char to token
		int len = strlen(newToken->content);
		if (len >= 1023) {
			// TODO - log warning
			return LEXER_INTERNAL_ERROR;
		}
		newToken->content[len] = c;
	}

	return LEXER_OK;
}

// Returns 0 on success
lexerResult lexStringToken(token* newToken) {
	// TODO - escape sequences
	newToken->type = TOKEN_STR_LITERAL;
	newToken->content = calloc(2048, sizeof(char));
	if (!newToken->content) {
		return 1;
	}

	int len = 0;
	while (1) {
		if (len >= 2047) {
			// TODO - emit warning
			return LEXER_INTERNAL_ERROR;
		}
		int c = getchar();
		if (c == EOF) {
			// TODO - emit warning
			return LEXER_ERROR;
		} else if (c == '"') {
			break;
		} else {
			newToken->content[len++] = c;
		}
	}

	return LEXER_OK;
}

lexerResult getNextToken(token* newToken) {
	assert(newToken);

	if (!BUFFERED_TOKEN_EMPTY) {
		*newToken = BUFFERED_TOKEN;
		BUFFERED_TOKEN_EMPTY = true;
		return LEXER_OK;
	}

	newToken->content = NULL;
	while (skipWhiteSpace() || skipComments()) {
	}

	int c = getchar();
	switch (c) {
		case EOF:
			newToken->type = TOKEN_EOF;
			return LEXER_OK;
		case '*':
			newToken->type = TOKEN_MUL;
			return LEXER_OK;
		case '/':
			newToken->type = TOKEN_DIV;
			return LEXER_OK;
		case '+':
			newToken->type = TOKEN_PLUS;
			return LEXER_OK;
		case ':':
			newToken->type = TOKEN_COLON;
			return LEXER_OK;
		case '(':
			newToken->type = TOKEN_BRACKET_ROUND_LEFT;
			return LEXER_OK;
		case ')':
			newToken->type = TOKEN_BRACKET_ROUND_RIGHT;
			return LEXER_OK;
		case '{':
			newToken->type = TOKEN_BRACKET_CURLY_LEFT;
			return LEXER_OK;
		case '}':
			newToken->type = TOKEN_BRACKET_CURLY_RIGHT;
			return LEXER_OK;
		case '=': {
			int c2 = getchar();
			if (c2 == '=') {
				newToken->type = TOKEN_EQ;
			} else {
				ungetc(c2, stdin);
				newToken->type = TOKEN_ASSIGN;
			}
			return LEXER_OK;
		}
		case '-': {
			int c2 = getchar();
			if (c2 == '>') {
				newToken->type = TOKEN_ARROW;
			} else {
				ungetc(c2, stdin);
				newToken->type = TOKEN_MINUS;
			}
			return LEXER_OK;
		}
		case '?': {
			int c2 = getchar();
			if (c2 == '?') {
				newToken->type = TOKEN_COALESCE;
			} else {
				ungetc(c2, stdin);
				newToken->type = TOKEN_QUESTION_MARK;
			}
			return LEXER_OK;
		}
		case '<': {
			int c2 = getchar();
			if (c2 == '=') {
				newToken->type = TOKEN_LESS_EQ;
			} else {
				ungetc(c2, stdin);
				newToken->type = TOKEN_LESS;
			}
			return LEXER_OK;
		}
		case '>': {
			int c2 = getchar();
			if (c2 == '=') {
				newToken->type = TOKEN_GREATER_EQ;
			} else {
				ungetc(c2, stdin);
				newToken->type = TOKEN_GREATER;
			}
			return LEXER_OK;
		}
		case '!': {
			int c2 = getchar();
			if (c2 == '=') {
				newToken->type = TOKEN_NEQ;
			} else {
				ungetc(c2, stdin);
				newToken->type = TOKEN_UNWRAP;
			}
			return LEXER_OK;
		}
		case '_': {
			int c2 = getchar();
			if (!isalnum(c2)) {
				newToken->type = TOKEN_UNDERSCORE;
			} else {
				ungetc(c2, stdin);
				ungetc(c, stdin);
				return lexIdentifierToken(newToken);
			}
			return LEXER_OK;
		}
		case '"':
			return lexStringToken(newToken);

		default:
			if (isalpha(c)) {
				ungetc(c, stdin);
				return lexIdentifierToken(newToken);
			} else if (isdigit(c)) {
				ungetc(c, stdin);
				return lexNumberToken(newToken);
			} else {
				return LEXER_ERROR;
			}
	}

	assert(0);
}
