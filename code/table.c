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
#if GROWTH_FACTOR == 2
	#define WRAP_VALUE(value, range) ((value) & ((range) - 1))
#else
	#define WRAP_VALUE(value, range) ((value) % (range))
#endif

	uint32_t index = WRAP_VALUE(key->hash, capacity);
	Entry* empty = NULL;
	for (uint32_t i = 0; i < capacity; i++) {
		Entry * entry = &entries[WRAP_VALUE(index + i, capacity)];
		if (entry->key == NULL) {
			if (empty == NULL) { empty = entry; }
			if (IS_NIL(entry->value)) { break; }
			continue;
		}
		if (entry->key == key) { return entry; }
		// rely on strings interning
		// if (entry->key->hash != key->hash) { continue; }
		// if (entry->key->length != key->length) { continue; }
		// if (memcmp(entry->key->chars, key->chars, sizeof(char) * key->length) != 0) { continue; }
		// return entry;
	}
	return empty;

#undef WRAP_VALUE
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

void table_add_all(Table * table, Table * from) {
	for (uint32_t i = 0; i < from->capacity; i++) {
		Entry * entry = &from->entries[i];
		if (entry->key == NULL) { continue; }
		table_set(table, entry->key, entry->value);
	}
}

struct Obj_String * table_find_key_copy(Table * table, char const * chars, uint32_t length, uint32_t hash) {
	// currently, it's compile-time and initialization-time function
	if (table->count == 0) { return false; }

	uint32_t index = hash % table->capacity;
	for (uint32_t i = 0; i < table->capacity; i++) {
		Entry * entry = &table->entries[(index + i) % table->capacity];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) { break; }
			continue;
		}
		if (entry->key->hash != hash) { continue; }
		if (entry->key->length != length) { continue; }
		if (memcmp(entry->key->chars, chars, sizeof(char) * length) != 0) { continue; }
		return entry->key;
	}
	return NULL;
}

struct Obj_String * table_find_key_concatenate(Table * table, char const * a_chars, uint32_t a_length, char const * b_chars, uint32_t b_length, uint32_t hash) {
	if (table->count == 0) { return false; }

	uint32_t length = a_length + b_length;

	uint32_t index = hash % table->capacity;
	for (uint32_t i = 0; i < table->capacity; i++) {
		Entry * entry = &table->entries[(index + i) % table->capacity];
		if (entry->key == NULL) {
			if (IS_NIL(entry->value)) { break; }
			continue;
		}
		if (entry->key->hash != hash) { continue; }
		if (entry->key->length != length) { continue; }
		if (memcmp(entry->key->chars, a_chars, sizeof(char) * a_length) != 0) { continue; }
		if (memcmp(entry->key->chars + a_length, b_chars, sizeof(char) * b_length) != 0) { continue; }
		return entry->key;
	}
	return NULL;
}

typedef struct Obj Obj;

void gc_mark_table_grey(Table * table) {
	for (uint32_t i = 0; i < table->capacity; i++) {
		Entry * entry = &table->entries[i];
		gc_mark_object_grey((Obj *)entry->key);
		gc_mark_value_grey(entry->value);
	}
}

void gc_table_remove_white_keys(Table * table) {
	for (uint32_t i = 0; i < table->capacity; i++) {
		Entry * entry = &table->entries[i];
		if (entry->key != NULL && !entry->key->obj.is_marked) {
			table_delete(table, entry->key);
		}
	}
}
