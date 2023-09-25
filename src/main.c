#include <stdio.h>

#include "analyser.h"
#include "ast.h"
#include "compiler.h"
#include "lexer.h"
#include "parser.h"

int main() {
	astProgram program;
	astProgramCreate(&program);

	parseProgram(&program);	   // TODO - check return value
	analyseProgram(&program);  // TODO - check return value
	compileProgram(&program);  // TODO - check return value

	astProgramDestroy(&program);
	return 0;
}
