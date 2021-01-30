#include <stdio.h>

#include "value.h"
#include "memory.h"

void value_print(Value value) {
	printf("%g", value);
}

void value_array_init(Value_Array * array) {
	array->count = 0;
	array->capacity = 0;
	array->values = NULL;
}

void value_array_free(Value_Array * array) {
	FREE_ARRAY(Value, array->values, array->capacity);
	value_array_init(array);
}

void value_array_write(Value_Array * array, Value value) {
	if (array->capacity < array->count + 1) {
		uint32_t old_capacity = array->capacity;
		array->capacity = GROW_CAPACITY(old_capacity);
		array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
	}

	array->values[array->count] = value;
	array->count++;
}
