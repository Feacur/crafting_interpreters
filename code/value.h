#if !defined(LOX_VALUE)
#define LOX_VALUE

#include "common.h"

typedef enum {
	VAL_NIL,
	VAL_NUMBER,
	VAL_BOOL,
} Value_Type;

typedef struct {
	Value_Type type;
	union {
		double number;
		bool boolean;
	} as;
} Value;

#define TO_NIL()         ((Value){VAL_NIL, {.number = 0.0}})
#define TO_NUMBER(value) ((Value){VAL_NUMBER, {.number = value}})
#define TO_BOOL(value)   ((Value){VAL_BOOL, {.boolean = value}})

#define AS_NIL           (NULL)
#define AS_NUMBER(value) (value.as.number)
#define AS_BOOL(value)   (value.as.boolean)

#define IS_NIL(value)    (value.type == VAL_NIL)
#define IS_NUMBER(value) (value.type == VAL_NUMBER)
#define IS_BOOL(value)   (value.type == VAL_BOOL)

typedef struct {
	uint32_t capacity, count;
	Value * values;
} Value_Array;

void value_print(Value value);

bool values_equal(Value a, Value b);

void value_array_init(Value_Array * array);
void value_array_free(Value_Array * array);
void value_array_write(Value_Array * array, Value value);

#endif
