#if !defined(LOX_VM)
#define LOX_VM

#include "value.h"

#define STACK_MAX 256

struct Chunk;

typedef struct {
	struct Chunk * chunk;
	uint8_t * ip;
	Value stack[STACK_MAX];
	Value * stack_top;
} VM;

typedef enum {
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
} Interpret_Result;

void vm_init(void);
void vm_free(void);
Interpret_Result vm_interpret_chunk(struct Chunk * chunk);
Interpret_Result vm_interpret(char const * source);
void vm_stack_push(Value value);
Value vm_stack_pop(void);
Value vm_stack_peek(uint32_t distance);

#endif
