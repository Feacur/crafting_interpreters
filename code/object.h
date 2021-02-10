#if !defined(LOX_OBJECT)
#define LOX_OBJECT

#include "chunk.h"

typedef enum {
	OBJ_STRING,
	OBJ_FUNCTION,
	OBJ_NATIVE,
} Obj_Type;

struct Obj {
	Obj_Type type;
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
	struct Chunk chunk;
	struct Obj_String * name;
};

struct Obj_Native {
	struct Obj obj;
	Native_Fn * function;
};

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define IS_FUNCTION(value) is_obj_type(value, OBJ_FUNCTION)
#define IS_NATIVE(value) is_obj_type(value, OBJ_NATIVE)

#define AS_STRING(value) ((struct Obj_String *)(void *)AS_OBJ(value))
#define AS_FUNCTION(value) ((struct Obj_Function *)(void *)AS_OBJ(value))
#define AS_NATIVE(value) ((struct Obj_Native *)(void *)AS_OBJ(value))

struct Obj_String * copy_string(char const * chars, uint32_t length);

inline static bool is_obj_type(Value value, Obj_Type type) {
	return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

void print_object(Value value);
struct Obj_String * strings_concatenate(Value a, Value b);

struct Obj_Function * new_function(void);
struct Obj_Native * new_native(Native_Fn * function);

void object_free(struct Obj * object);

#endif
