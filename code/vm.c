#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"

#if defined(DEBUG_TRACE_EXECUTION)
#include "debug.h"
#endif

static VM vm;

static void stack_reset(void) {
	vm.stack_top = vm.stack;
}

void vm_init(void) {
	stack_reset();
}

void vm_free(void) {

}

static Interpret_Result run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

#define OP_BINARY(op) \
	do { \
		Value b = vm_stack_pop(); \
		Value a = vm_stack_pop(); \
		vm_stack_push(a op b); \
	} while (false)

	for (;;) {
#if defined(DEBUG_TRACE_EXECUTION)
		printf("          ");
		for (Value * slot = vm.stack; slot < vm.stack_top; slot++) {
			printf("[ ");
			value_print(*slot);
			printf(" ]");
		}
		printf("\n");
		chunk_disassemble_instruction(vm.chunk, (uint32_t)(vm.ip - vm.chunk->code));
#endif // DEBUG_TRACE_EXECUTION

		uint8_t instruction;
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				vm_stack_push(constant);
				break;
			}

			case OP_ADD:      OP_BINARY(+); break;
			case OP_SUBTRACT: OP_BINARY(-); break;
			case OP_MULTIPLY: OP_BINARY(*); break;
			case OP_DIVIDE:   OP_BINARY(/); break;
			case OP_NEGATE:   vm_stack_push(-vm_stack_pop()); break;

			case OP_RETURN: {
				value_print(vm_stack_pop());
				printf("\n");
				return INTERPRET_OK;
			}
		}
	}

#undef READ_BYTE
#undef READ_CONSTANT
#undef OP_BINARY
}

typedef struct Chunk Chunk;

Interpret_Result vm_interpret_chunk(Chunk * chunk) {
	vm.chunk = chunk;
	vm.ip = vm.chunk->code;
	return run();
}

Interpret_Result vm_interpret(char const * source) {
	Chunk chunk;
	chunk_init(&chunk);

	if (!compile(source, &chunk)) {
		chunk_free(&chunk);
		return INTERPRET_COMPILE_ERROR;
	}

	vm.chunk = &chunk;
	vm.ip = vm.chunk->code;

	Interpret_Result result = run();

	chunk_free(&chunk);
	return result;
}

void vm_stack_push(Value value) {
	*vm.stack_top = value;
	vm.stack_top++;
}

Value vm_stack_pop(void) {
	vm.stack_top--;
	return *vm.stack_top;
}
