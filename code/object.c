#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, object_type) \
	(type *)(void *)allocate_object(sizeof(type), object_type)

typedef struct VM VM;

typedef struct Obj Obj;

static Obj * allocate_object(size_t size, Obj_Type type) {
	Obj * object = (Obj *)reallocate(NULL, 0, size);
	object->type = type;
	object->next = vm.objects;
	vm.objects = object;
	return object;
}

static Obj_String * allocate_string(char * chars, uint32_t length) {
	Obj_String * string = ALLOCATE_OBJ(Obj_String, OBJ_STRING);
	string->chars = chars;
	string->length = length;
	return string;
}

Obj_String * copy_string(char const * chars, uint32_t length) {
	char * heap_chars = ALLOCATE(char, length + 1);
	memcpy(heap_chars, chars, length);
	heap_chars[length] = '\0';
	return allocate_string(heap_chars, length);
}

void print_object(Value value) {
	switch (OBJ_TYPE(value)) {
		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;
	}
}

bool objects_equal(Value value_a, Value value_b) {
	if (OBJ_TYPE(value_a) != OBJ_TYPE(value_b)) { return false; }
	switch (OBJ_TYPE(value_a)) {
		case OBJ_STRING: {
			Obj_String * a_string = AS_STRING(value_a);
			Obj_String * b_string = AS_STRING(value_b);
			return a_string->length == b_string->length
			    && memcmp(a_string->chars, b_string->chars, sizeof(char) * a_string->length) == 0;
		}
	}
	return false; // unreachable
}

Obj_String * strings_concatenate(Value value_a, Value value_b) {
	Obj_String * a_string = AS_STRING(value_a);
	Obj_String * b_string = AS_STRING(value_b);
	uint32_t length = a_string->length + b_string->length;
	char * chars = ALLOCATE(char, length + 1);
	memcpy(chars, a_string->chars, sizeof(char) * a_string->length);
	memcpy(chars + a_string->length, b_string->chars, sizeof(char) * b_string->length);
	chars[length] = '\0';
	return allocate_string(chars, length);
}

void object_free(struct Obj * object) {
	switch (object->type) {
		case OBJ_STRING: {
			Obj_String * string = (Obj_String *)object;
			FREE_ARRAY(char, string->chars, string->length + 1);
			FREE(Obj_String, object);
			break;
		}
	}
}
