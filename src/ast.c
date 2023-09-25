#include "ast.h"

#include <stdlib.h>

void astProgramCreate(astProgram *program) {
	program->statements = NULL;
	program->count = 0;
}

/*
int astProgramAdd(astProgram* program, astTopLevelStatement statement) {
	if (program->statements == NULL) {
		//allocate memory for first statement
		program->statements = malloc(sizeof(statement));
		if (program->statements != NULL) {
			program->count = 1;
			return 0;
		} else {
			return 1;
		}
	}

	//push new statement to array
	if (!realloc(program->statements, (program->count + 1) * sizeof(statement))) {
		return 1;
	}

	program->statements[program->count] = statement;
	program->count++;
	return 0;
}
*/

void astProgramDestroy(astProgram *program) {
	if (program->statements) {
		free(program->statements);
	}
	program->count = 0;
}
