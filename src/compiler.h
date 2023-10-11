#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "symtable.h"

void compileProgram(const astProgram*, const symbolTable* functionTable);

#endif
