#if !defined(LOX_VM)
#define LOX_VM

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
	Chunk * chunk;
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
Interpret_Result vm_interpret(Chunk * chunk);
void vm_stack_push(Value value);
Value vm_stack_pop(void);

#endif
