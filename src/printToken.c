/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#include "printToken.h"

#include "assert.h"
#include "stdio.h"

void printToken(const token* tok, FILE* file) {
	assert(tok);
	switch (tok->type) {
		case TOKEN_EOF:
			fputs("EOF\n", file);
			break;
		case TOKEN_ASSIGN:
			fputs("=\n", file);
			break;
		case TOKEN_EQ:
			fputs("==\n", file);
			break;
		case TOKEN_NEQ:
			fputs("!=\n", file);
			break;
		case TOKEN_LESS:
			fputs("<\n", file);
			break;
		case TOKEN_GREATER:
			fputs(">\n", file);
			break;
		case TOKEN_LESS_EQ:
			fputs("<=\n", file);
			break;
		case TOKEN_GREATER_EQ:
			fputs(">=\n", file);
			break;
		case TOKEN_MUL:
			fputs("*\n", file);
			break;
		case TOKEN_DIV:
			fputs("/\n", file);
			break;
		case TOKEN_PLUS:
			fputs("+\n", file);
			break;
		case TOKEN_MINUS:
			fputs("-\n", file);
			break;
		case TOKEN_UNWRAP:
			fputs("!\n", file);
			break;
		case TOKEN_COALESCE:
			fputs("??\n", file);
			break;
		case TOKEN_ARROW:
			fputs("->\n", file);
			break;
		case TOKEN_BRACKET_ROUND_LEFT:
			fputs("(\n", file);
			break;
		case TOKEN_BRACKET_ROUND_RIGHT:
			fputs(")\n", file);
			break;
		case TOKEN_BRACKET_CURLY_LEFT:
			fputs("{\n", file);
			break;
		case TOKEN_BRACKET_CURLY_RIGHT:
			fputs("}\n", file);
			break;
		case TOKEN_COLON:
			fputs(":\n", file);
			break;
		case TOKEN_UNDERSCORE:
			fputs("_\n", file);
			break;
		case TOKEN_QUESTION_MARK:
			fputs("?\n", file);
			break;
		case TOKEN_KEYWORD_DOUBLE:
			fputs("Double\n", file);
			break;
		case TOKEN_KEYWORD_ELSE:
			fputs("else\n", file);
			break;
		case TOKEN_KEYWORD_FUNC:
			fputs("func\n", file);
			break;
		case TOKEN_KEYWORD_IF:
			fputs("if\n", file);
			break;
		case TOKEN_KEYWORD_INT:
			fputs("Int\n", file);
			break;
		case TOKEN_KEYWORD_LET:
			fputs("let\n", file);
			break;
		case TOKEN_KEYWORD_NIL:
			fputs("nil\n", file);
			break;
		case TOKEN_KEYWORD_RETURN:
			fputs("return\n", file);
			break;
		case TOKEN_KEYWORD_STRING:
			fputs("String\n", file);
			break;
		case TOKEN_KEYWORD_VAR:
			fputs("var\n", file);
			break;
		case TOKEN_KEYWORD_WHILE:
			fputs("while\n", file);
			break;
		case TOKEN_IDENTIFIER:
			fprintf(file, "IDENTIFIER: %s\n", tok->content);
			break;
		case TOKEN_INT_LITERAL:
			fprintf(file, "INTEGER LITERAL: %s\n", tok->content);
			break;
		case TOKEN_DEC_LITERAL:
			fprintf(file, "DECIMAL LITERAL: %s\n", tok->content);
			break;
		case TOKEN_STR_LITERAL:
			fprintf(file, "STRING LITERAL: %s\n", tok->content);
			break;
		case TOKEN_COMMA:
			fputs("/\n", file);
			break;
	}
}
