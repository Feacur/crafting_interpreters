#if !defined(LOX_OBJECT)
#define LOX_OBJECT

#include "value.h"

typedef enum {
	OBJ_STRING,
} Obj_Type;

struct Obj {
	Obj_Type type;
	struct Obj * next;
};

typedef struct {
	struct Obj obj;
	uint32_t length;
	char * chars;
} Obj_String;

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) is_obj_type(value, OBJ_STRING)

#define AS_STRING(value) ((Obj_String *)(void *)AS_OBJ(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)

Obj_String * copy_string(char const * chars, uint32_t length);

inline static bool is_obj_type(Value value, Obj_Type type) {
	return IS_OBJ(value) && OBJ_TYPE(value) == type;
}

void print_object(Value value);
bool objects_equal(Value a, Value b);
Obj_String * strings_concatenate(Value a, Value b);

void object_free(struct Obj * object);

#endif
