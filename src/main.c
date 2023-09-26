#include <stdio.h>

#include "analyser.h"
#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"

#define TEST_LEXER

#define END(value)                   \
	do {                             \
		astProgramDestroy(&program); \
		return value;                \
	} while (0)

int main() {
#ifdef TEST_LEXER
	while (1) {
		token tok;
		getNextToken(&tok);
		printToken(&tok);
		if (tok.type == TOKEN_EOF) {
			return 0;
		}
		tokenDestroy(&tok);
	}
#endif

	astProgram program;
	astProgramCreate(&program);

	switch (parseProgram(&program)) {
		case PARSE_LEXER_ERROR:
			END(1);
		case PARSE_ERROR:
			END(2);
		case PARSE_INTERNAL_ERROR:
			END(99);
		case PARSE_OK:
			break;
	}

	switch (analyseProgram(&program)) {
		case ANALYSIS_UNDEFINED_FUNC:
			END(3);
		case ANALYSIS_WRONG_FUNC_TYPE:
			END(4);
		case ANALYSIS_UNDEFINED_VAR:
			END(5);
		case ANALYSIS_WRONG_RETURN:
			END(6);
		case ANALYSIS_WRONG_BINARY_TYPES:
			END(7);
		case ANALYSIS_TYPE_DEDUCTION:
			END(8);
		case ANALYSIS_OTHER_ERROR:
			END(9);
		case ANALYSIS_OK:
			break;
	}

	compileProgram(&program);
	astProgramDestroy(&program);
	return 0;
}
