#include <stdio.h>

#include "object.h"
#include "memory.h"

inline static Value_Type value_type(Value value) {
#if defined(NAN_BOXING)
	if (IS_NIL(value)) { return VAL_NIL; }
	if (IS_NUMBER(value)) { return VAL_NUMBER; }
	if (IS_BOOL(value)) { return VAL_BOOL; }
	if (IS_OBJ(value)) { return VAL_OBJ; }
	return VAL_NIL;
#else
	return value.type;
#endif // NAN_BOXING
}

void value_print(Value value) {
	switch (value_type(value)) {
		case VAL_NIL:    printf("nil"); break;
		case VAL_NUMBER: printf("%g", AS_NUMBER(value)); break;
		case VAL_BOOL:   printf(AS_BOOL(value) ? "true" : "false"); break;
		case VAL_OBJ:    print_object(AS_OBJ(value)); break;
	}
}

typedef struct Obj Obj;

bool values_equal(Value a, Value b) {
#if defined(NAN_BOXING)
	// C# version disregards this; also it is not useful anyway
	// if (IS_NUMBER(a) && IS_NUMBER(b)) {
	// 	return AS_NUMBER(a) == AS_NUMBER(b);
	// }
	return a == b;
#else
	if (value_type(a) != value_type(b)) { return false; }
	switch (value_type(a)) {
		case VAL_NIL:    return true;
		case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
		case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
		case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);
	}
	return false; // unreachable
#endif // NAN_BOXING
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

void gc_mark_value_grey(Value value) {
	if (!IS_OBJ(value)) { return; }
	gc_mark_object_grey(AS_OBJ(value));
}

void gc_mark_value_array_grey(Value_Array * array) {
	for (uint32_t i = 0; i < array->count; i++) {
		gc_mark_value_grey(array->values[i]);
	}
}
