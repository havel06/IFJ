/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#ifndef PRINT_AST_H
#define PRINT_AST_H

#include <stdio.h>

#include "ast.h"

void printBinaryOperator(astBinaryOperator op, FILE*);
void astPrint(const astProgram*);

#endif
