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

// Returns 0 if no comments were skipped, -1 on error
static int skipComments() {
	int skipped = 0;
	int counter = 0;
	// single line comment
	while (true) {
		int slash1 = getchar();
		int slash2 = getchar();
		if (slash1 != '/' || slash2 != '/') {
			ungetc(slash2, stdin);
			ungetc(slash1, stdin);
			break;
		}

		// skip the comment
		while (true) {
			int c = getchar();
			if (c == '\n' || c == EOF) {
				break;
			} else {
				skipped++;
			}
		}
	}
	// mulitline comment
	while (true) {
		int slash = getchar();
		int asterisk = getchar();
		if (slash != '/' || asterisk != '*') {
			ungetc(asterisk, stdin);
			ungetc(slash, stdin);
			break;
		}
		int nextC = getchar();
		counter++;
		while (true) {	// find matching
			int c = nextC;
			nextC = getchar();
			if (c == EOF || nextC == EOF) {
				return -1;
			} else if (c == '/' && nextC == '*') {
				counter++;
			} else if (c == '*' && nextC == '/') {
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

// Returns the number of skipped spaces, 0 when none skipped, positive otherwise
static int skipWhiteSpace() {
	int skipped = 0;

	while (true) {
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

// Sets token to correct type if content matches keyword
static void checkForKeyword(token* newToken) {
	assert(newToken);
	assert(newToken->content);

	if (strcmp(newToken->content, "Int") == 0) {
		newToken->type = TOKEN_KEYWORD_INT;
	} else if (strcmp(newToken->content, "Double") == 0) {
		newToken->type = TOKEN_KEYWORD_DOUBLE;
	} else if (strcmp(newToken->content, "String") == 0) {
		newToken->type = TOKEN_KEYWORD_STRING;
	} else if (strcmp(newToken->content, "nil") == 0) {
		newToken->type = TOKEN_KEYWORD_NIL;
	} else if (strcmp(newToken->content, "var") == 0) {
		newToken->type = TOKEN_KEYWORD_VAR;
	} else if (strcmp(newToken->content, "let") == 0) {
		newToken->type = TOKEN_KEYWORD_LET;
	} else if (strcmp(newToken->content, "if") == 0) {
		newToken->type = TOKEN_KEYWORD_IF;
	} else if (strcmp(newToken->content, "else") == 0) {
		newToken->type = TOKEN_KEYWORD_ELSE;
	} else if (strcmp(newToken->content, "while") == 0) {
		newToken->type = TOKEN_KEYWORD_WHILE;
	} else if (strcmp(newToken->content, "func") == 0) {
		newToken->type = TOKEN_KEYWORD_FUNC;
	} else if (strcmp(newToken->content, "return") == 0) {
		newToken->type = TOKEN_KEYWORD_RETURN;
	}
}

static lexerResult lexIdentifierToken(token* newToken) {
	assert(newToken);
	newToken->type = TOKEN_IDENTIFIER;
	while (true) {
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

	enum { INT_PART, DEC_PART, EXP_PART, DEC_CHAR, EXP_CHAR, PM_CHAR } numberPart;
	numberPart = INT_PART;
	newToken->type = TOKEN_INT_LITERAL;

	while (true) {
		int c = getchar();

		switch (c) {
			case '.':
				if (numberPart != INT_PART) {
					return LEXER_ERROR;
				}
				numberPart = DEC_CHAR;
				newToken->type = TOKEN_DEC_LITERAL;
				break;
			case 'e':
			case 'E':
				if (numberPart == EXP_CHAR || numberPart == EXP_PART) {
					return LEXER_ERROR;
				}
				numberPart = EXP_CHAR;
				newToken->type = TOKEN_DEC_LITERAL;
				break;
			default:
				if (isalpha(c) || c == '_') {
					return LEXER_ERROR;
				}
				if (!isdigit(c)) {
					if ((c == '+' || c == '-') && numberPart == EXP_CHAR) {
						numberPart = PM_CHAR;
						break;
					}
					ungetc(c, stdin);
					if (!(numberPart == INT_PART || numberPart == DEC_PART || numberPart == EXP_PART)) {
						return LEXER_ERROR;
					}
					return LEXER_OK;
				}

				if (numberPart == DEC_CHAR) {
					numberPart = DEC_PART;
				}

				if (numberPart == PM_CHAR) {
					numberPart = EXP_PART;
				}

				if (numberPart == EXP_CHAR) {
					numberPart = EXP_PART;
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
			return LEXER_INTERNAL_ERROR;
		}
		newToken->content[len] = c;
	}

	return LEXER_OK;
}

bool isHexDigit(char c) { return isdigit(c) || ((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')); }

// Shifts content of string (starting at position 'start', of length 'len') to the left by 1
void leftShiftString(char* string, int start, int len) {
	for (int i = start; i < start + len - 1; i++) {
		string[i] = string[i + 1];
	}
	string[start + len - 1] = 0;
}

// Shifts content of string 'count' times
void leftShiftStringBy(char* string, int start, int len, int count) {
	for (int i = 0; i < count; i++) {
		leftShiftString(string, start, len);
		len--;
	}
}

static lexerResult lexEscapedChar(char* outChar) {
	int c = getchar();
	if (c == '\\') {
		*outChar = '\\';
	} else if (c == '"') {
		*outChar = '\"';
	} else if (c == 'n') {
		*outChar = '\n';
	} else if (c == 'r') {
		*outChar = '\r';
	} else if (c == 't') {
		*outChar = '\t';
	} else if (c == 'u') {
		// numbered ascii escape sequence
		char charCode[9];
		int charCodeLen = 0;

		int braceChar = getchar();
		if (braceChar != '{') {
			return LEXER_ERROR;
		}

		while (true) {
			int numChar = getchar();
			if (numChar == '}') {
				charCode[charCodeLen] = 0;
				break;
			} else if (isHexDigit(numChar)) {
				charCode[charCodeLen++] = numChar;
			} else {
				return LEXER_ERROR;
			}

			if (charCodeLen > 8) {
				return LEXER_ERROR;
			}
		}

		int code = strtol(charCode, NULL, 16);
		*outChar = code;
	} else {
		return LEXER_ERROR;
	}

	return LEXER_OK;
}

// Lexes mulitline strings that start and end with """
static lexerResult lexMultiLineStringToken(token* newToken) {
	int len = 0;
	while (true) {
		int c = getchar();
		// check for file end
		if (c == EOF) {
			return LEXER_ERROR;
		}

		if (c == '"') {
			// check for string end
			int c1 = getchar();
			int c2 = getchar();
			if (c1 == '"' && c2 == '"') {
				break;
			}
			ungetc(c2, stdin);
			ungetc(c1, stdin);
			newToken->content[len++] = c;
		} else if (c == '\\') {
			int ret = lexEscapedChar(&(newToken->content[len++]));
			if (ret == LEXER_ERROR) {
				return LEXER_ERROR;
			}
		} else {
			newToken->content[len++] = c;
		}

		if (len > 2047) {
			return LEXER_INTERNAL_ERROR;
		}
	}

	// get number of ignored spaces in indent
	int ignoredSpaces = 0;
	for (int i = len - 1; i > 0; i--) {
		if (newToken->content[i] == '\n') {
			break;
		} else if (newToken->content[i] == ' ') {
			ignoredSpaces++;
		} else {
			return LEXER_ERROR;	 // non-space symbol before closing quotes
		}
	}

	// remove whitespace and newline before closing quotes
	if (len > 0) {
		len -= ignoredSpaces + 1;
		newToken->content[len] = 0;
	}

	// remove ignored spaces from each line
	if (ignoredSpaces > 0) {
		// we start at -1 because first line does not have a newline in front
		for (int i = -1; i < len; i++) {
			if (i == -1 || newToken->content[i] == '\n') {
				// remove empty lines
				while (newToken->content[i + 1] == '\n') {
					// empty line
					i++;
				}

				// check that the line actually contains the indentation
				for (int j = 0; j < ignoredSpaces; j++) {
					if (newToken->content[i + j + 1] != ' ') {
						return LEXER_ERROR;
					}
				}
				leftShiftStringBy(newToken->content, i + 1, len - i - 1, ignoredSpaces);
				len -= ignoredSpaces;
			}
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

	if (cq1 == '"' && cq2 == '"' && cq3 == '\n') {
		return lexMultiLineStringToken(newToken);
	} else {
		ungetc(cq3, stdin);
		ungetc(cq2, stdin);
		ungetc(cq1, stdin);
	}

	int len = 0;
	while (true) {
		if (len >= 2047) {
			return LEXER_INTERNAL_ERROR;
		}
		int c = getchar();
		if (c == EOF || c <= 31 || c >= 127) {
			return LEXER_ERROR;
		} else if (c == '"') {
			break;
		} else if (c == '\\') {
			int ret = lexEscapedChar(&(newToken->content[len++]));

			if (ret == LEXER_ERROR) {
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

	while (true) {
		if (skipWhiteSpace()) {
			continue;
		}

		int skipCommentsResult = skipComments();
		if (skipCommentsResult == -1) {
			return LEXER_ERROR;
		} else if (skipCommentsResult > 0) {
			continue;
		}

		break;
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
				ungetc(c2, stdin);
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
