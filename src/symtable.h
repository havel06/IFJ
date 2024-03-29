/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h"

#define SYM_TABLE_CAPACITY 2048

typedef struct symbolTable symbolTable;	 // fwd

// Slot for variables
typedef struct {
	astDataType type;
	bool immutable;
	symbolTable* initialisedInScope;
} symbolVariable;

// Slot for functions
typedef struct {
	const astParameterList* params;	 // non-owning
	astDataType returnType;
} symbolFunc;

typedef struct {
	const char* name;  // non-owning
	bool taken;
	union {
		symbolVariable variable;
		symbolFunc function;
	};
	bool valid;	 // used in codegen for variable predefinitions
} symbolTableSlot;

struct symbolTable {
	symbolTableSlot data[SYM_TABLE_CAPACITY];  // TODO - make dynamic?
	int id;
};

typedef struct {
	symbolTable tables[256];  // TODO - make dynamic?
	int count;
} symbolTableStack;

void symTableCreate(symbolTable*);
bool symTableInsertVar(symbolTable*, symbolVariable, const char* name, bool valid);
bool symTableInsertFunc(symbolTable*, symbolFunc, const char* name);
symbolTableSlot* symTableLookup(symbolTable*, const char* name);

void symStackCreate(symbolTableStack*);
void symStackPush(symbolTableStack*);
void symStackPop(symbolTableStack*);
symbolTable* symStackCurrentScope(symbolTableStack*);
symbolTable* symStackGlobalScope(symbolTableStack*);
symbolTableSlot* symStackLookup(symbolTableStack*, const char* name, symbolTable** tablePtr);
void symStackValidate(symbolTableStack*, const char* name);	 // validate slot in symstack
void symStackSetVarType(symbolTableStack*, const char* name, astDataType type);

#endif
