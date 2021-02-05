#include <stdio.h>

#include "value.h"
#include "object.h"
#include "memory.h"

void value_print(Value value) {
	switch (value.type) {
		case VAL_NIL:    printf("nil"); break;
		case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
		case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false"); break;
		case VAL_OBJ:    print_object(value); break;
	}
}

typedef struct Obj Obj;

bool values_equal(Value a, Value b) {
	if (a.type != b.type) { return false; }
	switch (a.type) {
		case VAL_NIL:    return true;
		case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
		case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
		case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
	}
	return false; // unreachable
}

void value_array_init(Value_Array * array) {
	array->count = 0;
	array->capacity = 0;
	array->values = NULL;
}

void value_array_free(Value_Array * array) {
	FREE_ARRAY(array->values, array->capacity);
	value_array_init(array);
}

void value_array_write(Value_Array * array, Value value) {
	if (array->capacity < array->count + 1) {
		uint32_t old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(array->values, old_capacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;
}
