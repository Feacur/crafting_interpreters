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

#if defined(NAN_BOXING)
	typedef uint64_t Value;
#else
	typedef struct {
		Value_Type type;
		union {
			double number;
			bool boolean;
			struct Obj * obj;
		} as;
	} Value;
#endif // NAN_BOXING

typedef Value Native_Fn(uint8_t arg_count, Value * args);

#if defined(NAN_BOXING)
	inline static Value num_to_value(double number) {
		union {
			uint64_t bits;
			double number;
		} data;
		data.number = number;
		return data.bits;
	}

	inline static double value_to_num(Value bits) {
		union {
			uint64_t bits;
			double number;
		} data;
		data.bits = bits;
		return data.number;
	}

	#define TO_FALSE() ((Value)(uint64_t)(NAN_MASK | NAN_TAG_FALSE))
	#define TO_TRUE()  ((Value)(uint64_t)(NAN_MASK | NAN_TAG_TRUE))
#endif // NAN_BOXING

#if defined(NAN_BOXING)
	#define TO_NIL()          ((Value)(uint64_t)(NAN_MASK | NAN_TAG_NIL))
	#define TO_NUMBER(number) num_to_value(number)
	#define TO_BOOL(boolean)  ((boolean) ? TO_TRUE() : TO_FALSE())
	#define TO_OBJ(obj)       ((Value)(uint64_t)(NAN_SIGN | NAN_MASK | (uintptr_t)(obj)))

	#define AS_NIL           (NULL)
	#define AS_NUMBER(value) value_to_num(value)
	#define AS_BOOL(value)   ((value) == TO_TRUE())
	#define AS_OBJ(value)    ((struct Obj *)(uintptr_t)((value) & ~(NAN_SIGN | NAN_MASK)))

	#define IS_NIL(value)    ((value) == TO_NIL())
	#define IS_NUMBER(value) (((value) & NAN_MASK) != NAN_MASK)
	#define IS_BOOL(value)   (((value) | 1) == TO_TRUE())
	#define IS_OBJ(value)    (((value) & (NAN_SIGN | NAN_MASK)) == (NAN_SIGN | NAN_MASK))
#else
	#define TO_NIL()         ((Value){VAL_NIL,    {.obj     = NULL}})
	#define TO_NUMBER(value) ((Value){VAL_NUMBER, {.number  = value}})
	#define TO_BOOL(value)   ((Value){VAL_BOOL,   {.boolean = value}})
	#define TO_OBJ(value)    ((Value){VAL_OBJ,    {.obj     = (struct Obj *)(value)}})

	#define AS_NIL           (NULL)
	#define AS_NUMBER(value) ((value).as.number)
	#define AS_BOOL(value)   ((value).as.boolean)
	#define AS_OBJ(value)    ((value).as.obj)

	#define IS_NIL(value)    ((value).type == VAL_NIL)
	#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
	#define IS_BOOL(value)   ((value).type == VAL_BOOL)
	#define IS_OBJ(value)    ((value).type == VAL_OBJ)
#endif // NAN_BOXING

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
