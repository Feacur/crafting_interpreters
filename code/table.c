#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"

#define TABLE_MAX_LOAD 0.75

void table_init(Table * table) {
	table->count = 0;
	table->capacity = 0;
	table->entries = NULL;
}

void table_free(Table * table) {
	FREE_ARRAY(table->entries, table->capacity);
	table_init(table);
}

typedef struct Obj_String Obj_String;

static Entry * find_entry(Entry * entries, uint32_t capacity, Obj_String * key) {
	uint32_t index = key->hash % capacity;
	Entry* empty = NULL;
	for (uint32_t i = 0; i < capacity; i++) {
		Entry * entry = &entries[(index + i) % capacity];
		if (entry->key == key) { return entry; }
		if (entry->key == NULL) {
			if (empty == NULL) { empty = entry; }
			if (IS_NIL(entry->value)) { break; }
		}
	}
	return empty;
}

static void adjust_capacity(Table * table, uint32_t capacity) {
	Entry * entries = reallocate(NULL, 0, sizeof(Entry) * capacity);
	for (uint32_t i = 0; i < capacity; i++) {
		entries[i].key = NULL;
		entries[i].value = TO_NIL();
	}

	for (uint32_t i = 0; i < table->capacity; i++) {
		Entry * entry = &table->entries[i];
		if (entry->key == NULL) { continue; }

		Entry * dest = find_entry(entries, capacity, entry->key);
		dest->key = entry->key;
		dest->value = entry->value;
	}

	FREE_ARRAY(table->entries, table->capacity);
	table->entries = entries;
	table->capacity = capacity;
}

bool table_get(Table * table, Obj_String * key, Value * value) {
	if (table->count == 0) { return false; }

	Entry * entry = find_entry(table->entries, table->capacity, key);
	if (entry->key == NULL) { return false; }

	*value = entry->value;
	return true;
}

bool table_set(Table * table, Obj_String * key, Value value) {
	if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
		uint32_t capacity = GROW_CAPACITY(table->capacity);
		adjust_capacity(table, capacity);
	}

	Entry * entry = find_entry(table->entries, table->capacity, key);

	bool is_new_key = entry->key == NULL;
	if (is_new_key) { table->count++; }

	entry->key = key;
	entry->value = value;
	return is_new_key;
}

bool table_delete(Table * table, Obj_String * key) {
	if (table->count == 0) { return false; }

	Entry * entry = find_entry(table->entries, table->capacity, key);
	if (entry->key == NULL) { return false; }

	entry->key = NULL;
	entry->value = TO_BOOL(true);
	table->count--;

	return true;
}

void table_add_all(Table * table, Table * to) {
	for (uint32_t i = 0; i < table->capacity; i++) {
		Entry * entry = &table->entries[i];
		if (entry->key == NULL) { continue; }
		table_set(to, entry->key, entry->value);
	}
}
