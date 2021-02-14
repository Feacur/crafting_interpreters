#if !defined(LOX_VALUE)
#define LOX_VALUE

#include "common.h"

typedef enum {
	VAL_NIL,
	VAL_NUMBER,
	VAL_BOOL,
	VAL_OBJ,
} Value_Type;

struct Obj;

typedef struct {
	Value_Type type;
	union {
		double number;
		bool boolean;
		struct Obj * obj;
	} as;
} Value;

typedef Value Native_Fn(uint8_t arg_count, Value * args);

#define TO_NIL()         ((Value){VAL_NIL,    {.obj     = NULL}})
#define TO_NUMBER(value) ((Value){VAL_NUMBER, {.number  = value}})
#define TO_BOOL(value)   ((Value){VAL_BOOL,   {.boolean = value}})
#define TO_OBJ(value)    ((Value){VAL_OBJ,    {.obj     = (struct Obj *)value}})

#define AS_NIL           (NULL)
#define AS_NUMBER(value) ((value).as.number)
#define AS_BOOL(value)   ((value).as.boolean)
#define AS_OBJ(value)    ((value).as.obj)

#define IS_NIL(value)    ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_BOOL(value)   ((value).type == VAL_BOOL)
#define IS_OBJ(value)    ((value).type == VAL_OBJ)

typedef struct {
	uint32_t capacity, count;
	Value * values;
} Value_Array;

void value_print(Value value);

bool values_equal(Value a, Value b);

void value_array_init(Value_Array * array);
void value_array_free(Value_Array * array);
void value_array_write(Value_Array * array, Value value);

void gc_mark_value_grey(Value value);
void gc_mark_value_array_grey(Value_Array * array);

#endif
