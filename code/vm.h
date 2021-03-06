#if !defined(LOX_VM)
#define LOX_VM

#include "table.h"

struct Obj;

typedef struct {
	struct Obj * function;
	uint8_t * ip;
	Value * slots;
} Call_Frame;

struct Chunk;

struct VM {
	Call_Frame frames[FRAMES_MAX];
	uint32_t frame_count;

	Value stack[STACK_MAX];
	Value * stack_top;
	Table globals;
	Table strings;
	struct Obj_String * init_string;
	struct Obj_Upvalue * open_upvalues;
	struct Obj * objects;

	uint32_t greyCapacity, greyCount;
	struct Obj ** greyStack;

	size_t bytes_allocated;
	size_t next_gc;

	bool had_error;
};

extern struct VM vm;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} Interpret_Result;

void runtime_error(char const * format, ...);

void vm_init(void);
void vm_free(void);
Interpret_Result vm_interpret(char const * source);
void vm_stack_push(Value value);
Value vm_stack_pop(void);
Value vm_stack_peek(uint32_t distance);
void vm_stack_set(uint32_t distance, Value value);

struct Obj_Native;

void vm_define_native(char const * name, Native_Fn * function, uint8_t arity);

#endif
