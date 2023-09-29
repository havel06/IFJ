#include "symtable.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int LAST_TABLE_ID = 0;

static int hashFunc(const char* str) {
	int hash = 5381;
	int c;

	while ((c = *(str++))) {
		hash = hash * 33 + c;
	}

	return abs(hash) % 1024;
}

void symTableCreate(symbolTable* table) {
	for (int i = 0; i < 1024; i++) {
		table->data[i].taken = false;
	}
	table->id = LAST_TABLE_ID++;
}

static void symTableInsertSlot(symbolTable* table, symbolTableSlot slot) {
	int pos = hashFunc(slot.name);

	while (table->data[pos].taken) {
		pos++;
		assert(pos <= 1024);  // TODO - propagate error
	}

	table->data[pos] = slot;
}

void symTableInsertVar(symbolTable* table, symbolVariable var, const char* name) {
	symbolTableSlot slot;
	slot.variable = var;
	slot.name = name;
	slot.taken = true;
	symTableInsertSlot(table, slot);
}

void symTableInsertFunc(symbolTable* table, symbolFunc func, const char* name) {
	symbolTableSlot slot;
	slot.function = func;
	slot.name = name;
	slot.taken = true;
	symTableInsertSlot(table, slot);
}

symbolTableSlot* symTableLookup(symbolTable* table, const char* name) {
	int pos = hashFunc(name);

	while (pos <= 1023) {
		if (table->data[pos].taken == false) {
			return NULL;
		}

		if (strcmp(table->data[pos].name, name) == 0) {
			return &table->data[pos];
		}
		pos++;
	}

	return NULL;
}

void symStackCreate(symbolTableStack* stack) { stack->count = 0; }

void symStackPush(symbolTableStack* stack) { symTableCreate(&(stack->tables[stack->count++])); }

void symStackPop(symbolTableStack* stack) {
	// TODO - destroy symbol table if it contains allocated memory
	stack->count--;
}

symbolTable* symStackCurrentScope(symbolTableStack* stack) {
	assert(stack->count >= 0);
	return &(stack->tables[stack->count - 1]);
}

symbolTable* symStackGlobalScope(symbolTableStack* stack) {
	assert(stack->count >= 0);
	return &(stack->tables[0]);
}

symbolTableSlot* symStackLookup(symbolTableStack* stack, const char* name, symbolTable** tablePtr) {
	for (int i = stack->count - 1; i >= 0; i--) {
		symbolTableSlot* slot = symTableLookup(&stack->tables[i], name);
		if (slot) {
			if (tablePtr) {
				*tablePtr = &stack->tables[i];
			}
			return slot;
		}
	}

	return NULL;
}

// void symTableDestroy(symbolTable*);
