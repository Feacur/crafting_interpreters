#if !defined(LOX_VALUE)
#define LOX_VALUE

#include "common.h"

typedef double Value;

typedef struct {
	uint32_t capacity, count;
	Value * values;
} Value_Array;

void value_print(Value value);

void value_array_init(Value_Array * array);
void value_array_free(Value_Array * array);
void value_array_write(Value_Array * array, Value value);

#endif
