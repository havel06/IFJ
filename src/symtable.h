#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h"

typedef struct {
	astDataType type;
	bool immutable;
	bool initialised;
} symbolVariable;

typedef struct {
	// TODO
} symbolFunc;

typedef struct {
	const char* name;  // non-owning
	bool taken;
	union {
		symbolVariable variable;
		symbolFunc function;
	};
} symbolTableSlot;

typedef struct {
	symbolTableSlot data[1024];	 // TODO - make dynamic?
} symbolTable;

typedef struct {
	symbolTable tables[256];  // TODO - make dynamic?
	int count;
} symbolTableStack;

void symTableCreate(symbolTable*);
// void symTableDestroy(symbolTable*);
void symTableInsertVar(symbolTable*, symbolVariable, const char* name);
void symTableInsertFunc(symbolTable*, symbolFunc, const char* name);
symbolTableSlot* symTableLookup(symbolTable*, const char* name);

void symStackCreate(symbolTableStack*);
void symStackPush(symbolTableStack*);
void symStackPop(symbolTableStack*);
symbolTable* symStackCurrentScope(symbolTableStack*);
symbolTableSlot* symStackLookup(symbolTableStack*, const char* name, symbolTable** tablePtr);

#endif
