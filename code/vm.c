#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "object.h"
#include "chunk.h"
#include "compiler.h"
#include "memory.h"
#include "vm.h"

#if defined(DEBUG_TRACE_EXECUTION)
#include "debug.h"
#endif

typedef struct VM VM;
VM vm;

static void stack_reset(void) {
	vm.stack_top = vm.stack;
}

#if defined(__clang__) // clang: argument 1 of 2 is a printf-like format literal
__attribute__((format(printf, 1, 2)))
#endif // __clang__
static void runtime_error(char const * format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fputs("\n", stderr);

	size_t instruction = (size_t)(vm.ip - vm.chunk->code - 1);
	uint32_t line =  vm.chunk->lines[instruction];
	fprintf(stderr, "[line %d] in script\n", line);

	stack_reset();
}

void vm_init(void) {
	stack_reset();
	vm.objects = NULL;
}

void vm_free(void) {
	table_free(&vm.strings);
	objects_free();
}

static bool is_falsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static Interpret_Result run(void) {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

#define OP_BINARY(to_value, op) \
	do { \
		if (!IS_NUMBER(vm_stack_peek(0)) || !IS_NUMBER(vm_stack_peek(1))) { \
			runtime_error("operands must be numbers"); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		double b = AS_NUMBER(vm_stack_pop()); \
		double a = AS_NUMBER(vm_stack_pop()); \
		vm_stack_push(to_value(a op b)); \
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

			case OP_NIL:   vm_stack_push(TO_NIL()); break;
			case OP_FALSE: vm_stack_push(TO_BOOL(false)); break;
			case OP_TRUE:  vm_stack_push(TO_BOOL(true)); break;

			case OP_EQUAL: {
				Value b = vm_stack_pop();
				Value a = vm_stack_pop();
				vm_stack_push(TO_BOOL(values_equal(a, b)));
				break;
			}

			case OP_GREATER: OP_BINARY(TO_BOOL, >); break;
			case OP_LESS:    OP_BINARY(TO_BOOL, <); break;

			case OP_ADD: {
				if (IS_STRING(vm_stack_peek(0)) && IS_STRING(vm_stack_peek(1))) {
					Value b = vm_stack_pop();
					Value a = vm_stack_pop();
					vm_stack_push(TO_OBJ(strings_concatenate(a, b)));
				}
				else {
					OP_BINARY(TO_NUMBER, +);
				}
				break;
			}

			case OP_SUBTRACT: OP_BINARY(TO_NUMBER, -); break;
			case OP_MULTIPLY: OP_BINARY(TO_NUMBER, *); break;
			case OP_DIVIDE:   OP_BINARY(TO_NUMBER, /); break;

			case OP_NOT:
				vm_stack_push(TO_BOOL(is_falsey(vm_stack_pop()))); break;
			case OP_NEGATE:
				if (!IS_NUMBER(vm_stack_peek(0))) {
					runtime_error("operant must be a number");
					return INTERPRET_RUNTIME_ERROR;
				}
				vm_stack_push(TO_NUMBER(-AS_NUMBER(vm_stack_pop()))); break;

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

Value vm_stack_peek(uint32_t distance) {
	return vm.stack_top[-(int32_t)(distance + 1)];
}
