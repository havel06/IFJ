#ifndef ANALYSER_H
#define ANALYSER_H

#include "ast.h"

typedef enum {
	ANALYSIS_OK,
	ANALYSIS_UNDEFINED_FUNC,
	// TODO
	ANALYSIS_OTHER_ERROR
} analysisResult;

analysisResult analyseProgram(const astProgram*);

#endif
