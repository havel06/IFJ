/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

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

void tokenDestroy(token* tok) {
	assert(tok);
	if (tok->content) {
		free(tok->content);
		tok->content = NULL;
	}
}

// Returns 0 if no comments were skipped
static int skipComments() {
	int skipped = 0;
	int counter = 0;
	// single line comment
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
	// mulitline comment
	while (1) {
		int slash = getchar();
		int asterisk = getchar();
		if (slash != '/' || asterisk != '*') {
			ungetc(asterisk, stdin);
			ungetc(slash, stdin);
			break;
		}
		int nextC = getchar();
		counter++;
		while (1) {
			int c = nextC;
			nextC = getchar();
			if (c == '/' && nextC == '*') {
				counter++;
			}
			if (c == '*' && nextC == '/') {
				counter--;
				if (counter == 0) {
					break;
				}
				if (nextC == EOF) {
					break;
				}
			} else {
				skipped++;
			}
		}
	}

	return skipped;
}

// Returns 0 if no whitespace was skipped
static int skipWhiteSpace() {
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

static void checkForKeyword(token* newToken) {
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

static lexerResult lexIdentifierToken(token* newToken) {
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

static lexerResult lexNumberToken(token* newToken) {
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

bool isHexDigit(char c) { return isdigit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')); }

static lexerResult lexMultiLineStringToken(token* newToken) {
	int len = 0;
	while (true) {
		int c = getchar();
		// check for file end
		if (c == EOF) {
			return LEXER_ERROR;
		}

		// check for string end
		if (c == '\n') {
			int c1 = getchar();
			int c2 = getchar();
			int c3 = getchar();
			if (c1 == '\"' && c2 == '\"' && c3 == '\"') {
				break;
			} else {
				ungetc(c3, stdin);
				ungetc(c2, stdin);
				ungetc(c1, stdin);
			}
		}

		newToken->content[len++] = c;
		if (len > 2047) {
			// TODO - emit warning
			return LEXER_INTERNAL_ERROR;
		}
	}

	return LEXER_OK;
}

static lexerResult lexStringToken(token* newToken) {
	newToken->type = TOKEN_STR_LITERAL;
	newToken->content = calloc(2048, sizeof(char));
	if (!newToken->content) {
		return 1;
	}

	int cq1 = getchar();
	int cq2 = getchar();
	int cq3 = getchar();

	if (cq1 == '\"' && cq2 == '\"' && cq3 == '\n') {
		return lexMultiLineStringToken(newToken);
	} else {
		ungetc(cq3, stdin);
		ungetc(cq2, stdin);
		ungetc(cq1, stdin);
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
		} else if (c == '\\') {
			int c2 = getchar();
			if (c2 == '\\') {
				newToken->content[len++] = '\\';
			} else if (c2 == '"') {
				newToken->content[len++] = '\"';
			} else if (c2 == 'n') {
				newToken->content[len++] = '\n';
			} else if (c2 == 'r') {
				newToken->content[len++] = '\r';
			} else if (c2 == 't') {
				newToken->content[len++] = '\t';
			} else if (c2 == 'u') {
				// numbered ascii escape sequence
				char charCode[8];
				int charCodeLen = 0;

				while (charCodeLen < 8) {
					int numChar = getchar();
					if (isHexDigit(numChar)) {
						charCode[charCodeLen++] = numChar;
					} else {
						charCode[charCodeLen] = '\0';
						ungetc(numChar, stdin);
						break;
					}
				}

				int code = strtol(charCode, NULL, 16);
				newToken->content[len++] = code;
			} else {
				return LEXER_ERROR;
			}
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
		case ',':
			newToken->type = TOKEN_COMMA;
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
