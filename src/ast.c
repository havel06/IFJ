#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void astProgramCreate(astProgram *program) {
	program->statements = NULL;
	program->count = 0;
}

int astProgramAdd(astProgram *program, astTopLevelStatement statement) {
	if (program->statements == NULL) {
		// allocate memory for first statement
		program->statements = malloc(sizeof(statement));
		if (program->statements != NULL) {
			program->count = 1;
			program->statements[0] = statement;
			return 0;
		} else {
			return 1;
		}
	}

	// push new statement to array
	program->statements = realloc(program->statements, (program->count + 1) * sizeof(statement));
	if (program->statements == NULL) {
		return 1;
	}

	program->statements[program->count] = statement;
	program->count++;
	return 0;
}

void astProgramDestroy(astProgram *program) {
	// TODO - recurse down the program and clear all nodes
	if (program->statements) {
		free(program->statements);
		program->statements = NULL;
	}
	program->count = 0;
}

int astVarDefCreate(astVariableDefinition *varDef, const char *str, astDataType type, astExpression expr,
					bool immutable) {
	varDef->variableName = malloc(strlen(str) * sizeof(char));
	if (varDef->variableName == NULL) {
		return 1;
	}

	varDef->variableType = type;
	varDef->value = expr;
	varDef->immutable = immutable;
	return 0;
}

/*
void astVarDefDestroy(astVariableDefinition* varDef) {
	if (varDef->variableName) {
		free(varDef->variableName);
	}
}
*/
