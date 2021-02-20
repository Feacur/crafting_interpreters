#if !defined(LOX_OBJECT)
#define LOX_OBJECT

#include "chunk.h"
#include "table.h"

typedef enum {
	OBJ_STRING,
	OBJ_NATIVE,
	OBJ_FUNCTION,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_CLASS,
	OBJ_INSTANCE,
	OBJ_BOUND_METHOD,
} Obj_Type;

struct Obj {
	Obj_Type type;
	bool is_marked;
	struct Obj * next;
};

struct Obj_String {
	struct Obj obj;
	uint32_t hash;
	uint32_t length;
	char chars[FLEXIBLE_ARRAY];
};

struct Obj_Function {
	struct Obj obj;
	uint8_t arity;
	uint32_t upvalue_count;
	struct Chunk chunk;
	struct Obj_String * name;
};

struct Obj_Native {
	struct Obj obj;
	Native_Fn * function;
	uint8_t arity;
};

struct Obj_Upvalue {
	struct Obj obj;
	Value closed;
	Value * location;
	struct Obj_Upvalue * next;
};

struct Obj_Closure {
	struct Obj obj;
	struct Obj_Function * function;
	struct Obj_Upvalue ** upvalues;
	uint32_t upvalue_count;
};

struct Obj_Class {
	struct Obj obj;
	struct Obj_String * name;
	Table methods;
};

struct Obj_Instance {
	struct Obj obj;
	struct Obj_Class * lox_class;
	Table table;
};

struct Obj_Bound_Method {
	struct Obj obj;
	Value receiver;
	struct Obj_Function * method;
};

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)
#define IS_CLOSURE(value) is_obj_type(value, OBJ_CLOSURE)
#define IS_UPVALUE(value) is_obj_type(value, OBJ_UPVALUE)
#define IS_CLASS(value) is_obj_type(value, OBJ_CLASS)
#define IS_INSTANCE(value) is_obj_type(value, OBJ_INSTANCE)
#define IS_BOUND_METHOD(value) is_obj_type(value, OBJ_BOUND_METHOD)

#define AS_STRING(value) ((struct Obj_String *)(void *)AS_OBJ(value))
#define AS_FUNCTION(value) ((struct Obj_Function *)(void *)AS_OBJ(value))
#define AS_NATIVE(value) ((struct Obj_Native *)(void *)AS_OBJ(value))
#define AS_CLOSURE(value) ((struct Obj_Closure *)(void *)AS_OBJ(value))
#define AS_UPVALUE(value) ((struct Obj_Upvalue *)(void *)AS_OBJ(value))
#define AS_CLASS(value) ((struct Obj_Class *)(void *)AS_OBJ(value))
#define AS_INSTANCE(value) ((struct Obj_Instance *)(void *)AS_OBJ(value))
#define AS_BOUND_METHOD(value) ((struct Obj_Bound_Method *)(void *)AS_OBJ(value))

struct Obj_String * copy_string(char const * chars, uint32_t length);

inline static bool is_obj_type(Value value, Obj_Type type) {
	return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

void print_object(struct Obj * object);
struct Obj_String * strings_concatenate(struct Obj_String * a, struct Obj_String * b);

struct Obj_Function * new_function(void);
struct Obj_Native * new_native(Native_Fn * function, uint8_t arity);
struct Obj_Closure * new_closure(struct Obj_Function * function);
struct Obj_Upvalue * new_upvalue(Value * slot);
struct Obj_Class * new_class(struct Obj_String * name);
struct Obj_Instance * new_instance(struct Obj_Class * lox_class);
struct Obj_Bound_Method * new_bound_method(Value receiver, struct Obj_Function * method);

void gc_free_object(struct Obj * object);

void gc_mark_object_grey(struct Obj * object);
void gc_mark_object_black(struct Obj * object);

#endif
