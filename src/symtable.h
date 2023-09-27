#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "ast.h"

typedef struct {
	const char* name;  // non-owning
	astDataType type;
	bool immutable;
} symbolVariable;

typedef struct {
	bool taken;
	symbolVariable value;
} symbolTableScopeSlot;

typedef struct {
	symbolTableScopeSlot data[1024];  // TODO - make dynamic
} symbolTableScope;

typedef struct {
	symbolTableScope scopes[128];  // TODO - make dynamic
	int count;
} symbolTable;

void symTableCreate(symbolTable*);
// void symTableDestroy(symbolTable*);
void symTablePushScope(symbolTable*);
void symTablePopScope(symbolTable*);
void symTableInsert(symbolTable*, symbolVariable);
symbolVariable* symTableLookup(symbolTable*, const char* name);

#endif
