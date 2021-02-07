#if !defined(LOX_VM)
#define LOX_VM

#include "table.h"

struct Obj_Function;

typedef struct {
	struct Obj_Function * function;
	uint8_t * ip;
	Value * slots;
} Call_Frame;

struct Chunk;

struct VM {
	Call_Frame frames[MAX_FRAMES];
	uint32_t frames_count;

	Value stack[STACK_MAX];
	Value * stack_top;
	Table globals;
	Table strings;
	struct Obj * objects;
};

extern struct VM vm;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} Interpret_Result;

void vm_init(void);
void vm_free(void);
Interpret_Result vm_interpret(char const * source);
void vm_stack_push(Value value);
Value vm_stack_pop(void);
Value vm_stack_peek(uint32_t distance);

#endif
