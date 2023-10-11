#ifndef ANALYSER_H
#define ANALYSER_H

#include "ast.h"
#include "symtable.h"

typedef enum {
	ANALYSIS_OK,
	ANALYSIS_UNDEFINED_FUNC,
	ANALYSIS_WRONG_FUNC_TYPE,
	ANALYSIS_UNDEFINED_VAR,
	ANALYSIS_WRONG_RETURN,
	ANALYSIS_WRONG_BINARY_TYPES,
	ANALYSIS_TYPE_DEDUCTION,
	ANALYSIS_OTHER_ERROR
} analysisResult;

analysisResult analyseProgram(const astProgram*, symbolTable* functionTable);

#endif
