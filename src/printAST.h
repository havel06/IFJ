#ifndef PRINT_AST_H
#define PRINT_AST_H

#include <stdio.h>

#include "ast.h"

void printBinaryOperator(astBinaryOperator op, FILE*);
void astPrint(const astProgram*);

#endif
