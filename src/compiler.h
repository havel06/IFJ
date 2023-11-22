/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "symtable.h"

void compileProgram(const astProgram*, const symbolTable* functionTable);

#endif
