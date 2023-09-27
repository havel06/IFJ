#include "symtable.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*
static int hashFunc(const char* str) {
	int hash = 5381;
	int c;

	while ((c = *(str++))) {
		hash = hash * 33 + c;
	}

	return hash % 1024;
}

void symTableCreate(symbolTable* table) {
	table->count = 0;
}

void symTablePushScope(symbolTable* table) {
	for (int i = 0; i < 1024; i++) {
		table->scopes[table->count].data[i].taken = false;
	}
	table->count++;
	assert(table->count <= 128); //TODO - propagate error
}

void symTablePopScope(symbolTable* table) {
	table->count--;
}

void symTableInsert(symbolTable* table, symbolVariable variable) {
	symbolTableScope* scope = &(table->scopes[table->count - 1]);
	int pos = hashFunc(variable.name);

	while (scope->data[pos].taken) {
		pos++;
		assert(pos <= 1024); //TODO - propagate error
	}

	scope->data[pos].taken = true;
	scope->data[pos].value = variable;
}

symbolVariable* symTableLookup(symbolTable* table, const char* name) {
	for (int i = table->count - 1; i >= 0; i--) {
		symbolTableScope* scope = &(table->scopes[table->count - 1]);
		int pos = hashFunc(name);

		while (pos <= 1023) {
			if (strcmp(scope->data[pos].value.name, name) == 0) {
				return &scope->data[pos].value;
			}
			pos++;
		}
	}

	return NULL;
}
*/

// void symTableDestroy(symbolTable* table) {
//
// }
