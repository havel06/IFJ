#include <stdio.h>

#include "analyser.h"
#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "printToken.h"

// edit these two
#define TEST_LEXER
#define TEST_PARSER

#ifdef TEST_PARSER
#undef TEST_LEXER
#endif

#define END(value)                   \
	do {                             \
		astProgramDestroy(&program); \
		return value;                \
	} while (0)

int main() {
#ifdef TEST_LEXER
	while (1) {
		token tok;
		tok.content = NULL;
		int status = getNextToken(&tok);
		if (status) {
			fprintf(stderr, "Unexpected symbol: %c\n", getchar());
		}
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

#ifdef TEST_PARSER
	astPrint(&program);
#endif

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
