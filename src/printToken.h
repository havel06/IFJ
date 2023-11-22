/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#ifndef PRINT_TOKEN_H
#define PRINT_TOKEN_H

#include <stdio.h>

#include "lexer.h"

void printToken(const token*, FILE*);

#endif
