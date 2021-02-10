#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
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
	vm.frame_count = 0;
}

typedef struct Obj_Function Obj_Function;
typedef struct Obj_Native Obj_Native;

#if defined(__clang__) // clang: argument 1 of 2 is a printf-like format literal
__attribute__((format(printf, 1, 2)))
#endif // __clang__
void runtime_error(char const * format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fputs("\n", stderr);

	for (uint32_t i = vm.frame_count; i-- > 0;) {
		Call_Frame * frame = &vm.frames[i];
		Obj_Function * function = frame->function;
		size_t instruction = (size_t)(frame->ip - function->chunk.code - 1);
		fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
		if (function->name == NULL) {
			fprintf(stderr, "script\n");
		}
		else {
			fprintf(stderr, "%s()\n", function->name->chars);
		}
	}

	stack_reset();
	vm.had_error = true;
}

void vm_init(void) {
	stack_reset();
	vm.objects = NULL;
	vm.had_error = false;
	table_init(&vm.globals);
	table_init(&vm.strings);
}

void vm_free(void) {
	table_free(&vm.globals);
	table_free(&vm.strings);
	objects_free();
}

static bool is_falsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static bool call_function(Obj_Function * function, uint8_t arg_count) {
	if (arg_count != function->arity) {
		runtime_error("expected %d arguments, but got %d", function->arity, arg_count);
		return false;
	}

	if (vm.frame_count == FRAMES_MAX) {
		runtime_error("stack overflow");
		return false;
	}

	Call_Frame * frame = &vm.frames[vm.frame_count++];
	frame->function = function;
	frame->ip = function->chunk.code;

	frame->slots = vm.stack_top - arg_count - 1;

	return true;
}

static bool call_native(Obj_Native * native, uint8_t arg_count) {
	if (arg_count != native->arity) {
		runtime_error("expected %d arguments, but got %d", native->arity, arg_count);
		return false;
	}

	Value result = native->function(arg_count, vm.stack_top - arg_count);
	vm.stack_top -= arg_count + 1;
	vm_stack_push(result);

	return !vm.had_error;
}

static bool call_value(Value callee, uint8_t arg_count) {
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
			case OBJ_FUNCTION:
				return call_function(AS_FUNCTION(callee), arg_count);
			case OBJ_NATIVE:
				return call_native(AS_NATIVE(callee), arg_count);
			default: break;
		}
	}

	runtime_error("can only call functions and classes");
	return false;
}

typedef struct Obj_String Obj_String;

static Interpret_Result run(void) {
	Call_Frame * frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*(frame->ip++))
#define READ_SHORT() (frame->ip += 2, (uint16_t)(frame->ip[-2] << 8) | (uint16_t)frame->ip[-1])
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

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
		printf("  stack:  ");
		for (Value * slot = vm.stack; slot < vm.stack_top; slot++) {
			printf("[ ");
			value_print(*slot);
			printf(" ]");
		}
		printf("\n");
		chunk_disassemble_instruction(&frame->function->chunk, (uint32_t)(frame->ip - frame->function->chunk.code));
#endif // DEBUG_TRACE_EXECUTION

		Op_Code instruction;
		switch (instruction = READ_BYTE()) {
			case OP_CONSTANT: {
				Value constant = READ_CONSTANT();
				vm_stack_push(constant);
				break;
			}

			case OP_NIL:   vm_stack_push(TO_NIL()); break;
			case OP_FALSE: vm_stack_push(TO_BOOL(false)); break;
			case OP_TRUE:  vm_stack_push(TO_BOOL(true)); break;

			case OP_POP: vm_stack_pop(); break;

			case OP_SET_LOCAL: {
				uint8_t slot = READ_BYTE();
				frame->slots[slot] = vm_stack_peek(0);
				break;
			}

			case OP_GET_LOCAL: {
				uint8_t slot = READ_BYTE();
				vm_stack_push(frame->slots[slot]);
				break;
			}

			case OP_SET_GLOBAL: {
				Obj_String * name = READ_STRING();
				if (table_set(&vm.globals, name, vm_stack_peek(0))) {
					table_delete(&vm.globals, name);
					runtime_error("undefined variable '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

			case OP_GET_GLOBAL: {
				Obj_String * name = READ_STRING();
				Value value;
				if (!table_get(&vm.globals, name, &value)) {
					runtime_error("undefined variable '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				vm_stack_push(value);
				break;
			}

			case OP_DEFINE_GLOBAL: {
				Obj_String * name = READ_STRING();
				table_set(&vm.globals, name, vm_stack_peek(0));
				vm_stack_pop();
				break;
			}

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

			case OP_NOT: vm_stack_push(TO_BOOL(is_falsey(vm_stack_pop()))); break;
			case OP_NEGATE: {
				if (!IS_NUMBER(vm_stack_peek(0))) {
					runtime_error("operant must be a number");
					return INTERPRET_RUNTIME_ERROR;
				}
				vm_stack_push(TO_NUMBER(-AS_NUMBER(vm_stack_pop())));
				break;
			}

			case OP_LOOP: {
				uint16_t offset = READ_SHORT();
				frame->ip -= offset;
				break;
			}

			case OP_JUMP: {
				uint16_t offset = READ_SHORT();
				frame->ip += offset;
				break;
			}

			case OP_JUMP_IF_FALSE: {
				uint16_t offset = READ_SHORT();
				frame->ip += offset * is_falsey(vm_stack_peek(0));
				break;
			}

			case OP_CALL: {
				uint8_t arg_count = READ_BYTE();
				if (!call_value(vm_stack_peek(arg_count), arg_count)) {
					return INTERPRET_RUNTIME_ERROR;
				}
				frame = &vm.frames[vm.frame_count - 1];
				break;
			}

			case OP_RETURN: {
				Value result = vm_stack_pop();

				vm.frame_count--;
				if (vm.frame_count == 0) {
					vm_stack_pop();
					return INTERPRET_OK;
				}

				vm.stack_top = frame->slots;
				vm_stack_push(result);

				frame = &vm.frames[vm.frame_count - 1];
				break;
			}
		}
	}

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef OP_BINARY
}

typedef struct Chunk Chunk;

typedef struct Obj Obj;

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

Interpret_Result vm_interpret(char const * source) {
	Obj_Function * function = compile(source);
	if (function == NULL) { return INTERPRET_COMPILE_ERROR; }

	vm_stack_push(TO_OBJ(function));
	call_function(function, 0);

	return run();
}

void vm_define_native(char const * name, Native_Fn * function, uint8_t arity) {
	vm_stack_push(TO_OBJ(copy_string(name, (uint32_t)strlen(name))));
	vm_stack_push(TO_OBJ(new_native(function, arity)));
	table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
	vm_stack_pop();
	vm_stack_pop();
}
