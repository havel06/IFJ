#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h"

#define SYM_TABLE_CAPACITY 1024

typedef struct symbolTable symbolTable;	 // fwd

typedef struct {
	astDataType type;
	bool immutable;
	symbolTable* initialisedInScope;
} symbolVariable;

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
// void symTableDestroy(symbolTable*);
bool symTableInsertVar(symbolTable*, symbolVariable, const char* name);
bool symTableInsertFunc(symbolTable*, symbolFunc, const char* name);
symbolTableSlot* symTableLookup(symbolTable*, const char* name);

void symStackCreate(symbolTableStack*);
void symStackPush(symbolTableStack*);
void symStackPop(symbolTableStack*);
symbolTable* symStackCurrentScope(symbolTableStack*);
symbolTable* symStackGlobalScope(symbolTableStack*);
symbolTableSlot* symStackLookup(symbolTableStack*, const char* name, symbolTable** tablePtr);

#endif
