#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, flexible, object_type) \
	(type *)(void *)allocate_object(sizeof(type) + flexible, object_type)

#define FREE_OBJ(pointer, flexible) \
	reallocate(pointer, sizeof(*pointer) + flexible, 0)

typedef struct VM VM;

typedef struct Obj Obj;

static Obj * allocate_object(size_t size, Obj_Type type) {
	Obj * object = (Obj *)reallocate(NULL, 0, size);
	object->type = type;
	object->next = vm.objects;
	vm.objects = object;
	return object;
}

typedef struct Obj_String Obj_String;

static Obj_String * allocate_string(uint32_t length) {
	Obj_String * string = ALLOCATE_OBJ(Obj_String, sizeof(char) * (length + 1), OBJ_STRING);
	string->length = length;
	string->chars[length] = '\0';
	return string;
}

inline static uint32_t hash_string_2(uint32_t hash, char const * chars, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
		hash ^= (uint8_t)chars[i];
		hash *= 16777619u;
	}
	return hash;
}

static uint32_t hash_string(char const * chars, uint32_t length) {
	// FNV-1a
	// http://www.isthe.com/chongo/tech/comp/fnv/
	// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
	return hash_string_2(2166136261u, chars, length);
}

Obj_String * copy_string(char const * chars, uint32_t length) {
	uint32_t hash = hash_string(chars, length);
	Obj_String * interned = table_find_key(&vm.strings, chars, length, hash);
	if (interned != NULL) { return interned; }

	Obj_String * string = allocate_string(length);
	memcpy(string->chars, chars, length);
	string->hash = hash;

	table_set(&vm.strings, string, TO_NIL());
	return string;
}

typedef struct Obj_Function Obj_Function;

void print_object(Value value) {
	switch (OBJ_TYPE(value)) {
		case OBJ_STRING:
			printf("%s", AS_STRING(value)->chars);
			break;
		case OBJ_FUNCTION: {
			Obj_Function * function = AS_FUNCTION(value);
			if (function->name == NULL) {
				printf("<script>");
			}
			else {
				printf("<fn %s>", AS_FUNCTION(value)->name->chars);
			}
			break;
		}
	}
}

Obj_String * strings_concatenate(Value value_a, Value value_b) {
	Obj_String * a_string = AS_STRING(value_a);
	Obj_String * b_string = AS_STRING(value_b);

	uint32_t hash = hash_string_2(a_string->hash, b_string->chars, b_string->length);
	uint32_t length = a_string->length + b_string->length;
	Obj_String * interned = table_find_key_2(&vm.strings, a_string->chars, a_string->length, b_string->chars, b_string->length, hash);
	if (interned != NULL) { return interned; }

	Obj_String * string = allocate_string(length);
	memcpy(string->chars, a_string->chars, sizeof(char) * a_string->length);
	memcpy(string->chars + a_string->length, b_string->chars, sizeof(char) * b_string->length);
	string->hash = hash;

	table_set(&vm.strings, string, TO_NIL());
	return string;
}

Obj_Function * new_function(void) {
	Obj_Function * function = ALLOCATE_OBJ(Obj_Function, 0, OBJ_FUNCTION);
	function->arity = 0;
	function->name = NULL;
	chunk_init(&function->chunk);
	return function;
}

void object_free(struct Obj * object) {
	switch (object->type) {
		case OBJ_STRING: {
			Obj_String * string = (Obj_String *)object;
			FREE_OBJ(string, sizeof(char) * string->length);
			break;
		}
		case OBJ_FUNCTION: {
			Obj_Function * function = (Obj_Function *)object;
			chunk_free(&function->chunk);
			FREE_OBJ(function, 0);
			break;
		}
	}
}
