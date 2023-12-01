/*
 * Implementace překladače imperativního jazyka IFJ23
 *
 * Michal Havlíček (xhavli65)
 * Adam Krška (xkrska08)
 * Tomáš Sitarčík (xsitar06)
 * Jan Šemora (xsemor01)
 *
 */

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

	return abs(hash) % SYM_TABLE_CAPACITY;
}

void symTableCreate(symbolTable* table) {
	for (int i = 0; i < SYM_TABLE_CAPACITY; i++) {
		table->data[i].taken = false;
	}
	table->id = LAST_TABLE_ID++;
}

static bool symTableInsertSlot(symbolTable* table, symbolTableSlot slot) {
	int pos = hashFunc(slot.name);
	const int startingPos = pos;

	while (table->data[pos].taken) {
		if (strcmp(table->data[pos].name, slot.name) == 0) {
			return false;  // redefinition
		}
		pos++;
		pos %= SYM_TABLE_CAPACITY;
		if (pos == startingPos) {
			// already searched the full table
			return false;
		}
	}

	table->data[pos] = slot;
	return true;
}

bool symTableInsertVar(symbolTable* table, symbolVariable var, const char* name, bool valid) {
	symbolTableSlot slot;
	slot.variable = var;
	slot.name = name;
	slot.taken = true;
	slot.valid = valid;
	return symTableInsertSlot(table, slot);
}

bool symTableInsertFunc(symbolTable* table, symbolFunc func, const char* name) {
	symbolTableSlot slot;
	slot.function = func;
	slot.name = name;
	slot.taken = true;
	slot.valid = true;
	return symTableInsertSlot(table, slot);
}

symbolTableSlot* symTableLookup(symbolTable* table, const char* name) {
	int pos = hashFunc(name);
	const int startingPos = pos;

	while (true) {
		if (table->data[pos].taken == false) {
			return NULL;
		}

		if (strcmp(table->data[pos].name, name) == 0) {
			return &table->data[pos];
		}

		pos++;
		pos %= SYM_TABLE_CAPACITY;
		if (pos == startingPos) {
			// already searched the whole table
			return NULL;
		}
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
		if (slot && slot->valid) {
			if (tablePtr) {
				*tablePtr = &stack->tables[i];
			}
			return slot;
		}
	}

	return NULL;
}

void symStackValidate(symbolTableStack* stack, const char* name) {
	for (int i = stack->count - 1; i >= 0; i--) {
		symbolTableSlot* slot = symTableLookup(&stack->tables[i], name);
		if (slot) {
			slot->valid = true;
			return;
		}
	}

	assert(false);
}

// void symTableDestroy(symbolTable*);
