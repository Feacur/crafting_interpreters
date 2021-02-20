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
#endif // DEBUG_TRACE_EXECUTION

typedef struct VM VM;
VM vm;

static void stack_reset(void) {
	vm.stack_top = vm.stack;
	vm.frame_count = 0;
	vm.open_upvalues = NULL;
}

typedef struct Obj_Function Obj_Function;
typedef struct Obj_Closure Obj_Closure;
typedef struct Obj_Bound_Method Obj_Bound_Method;

inline static Obj_Function * get_frame_function(Call_Frame * frame) {
	switch (frame->function->type) {
		case OBJ_FUNCTION:
			return (Obj_Function *)frame->function;
		// case OBJ_CLOSURE:
		default:
			return ((Obj_Closure *)frame->function)->function;
	}
	// return NULL;
}

#if defined(__clang__) // clang: argument 1 of 2 is a printf-like format literal
__attribute__((format(printf, 1, 2)))
#endif // __clang__
void runtime_error(char const * format, ...) {
	DEBUG_BREAK();
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fputs("\n", stderr);

	for (uint32_t i = vm.frame_count; i-- > 0;) {
		Call_Frame * frame = &vm.frames[i];
		Obj_Function * function = get_frame_function(frame);
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

	vm.greyCapacity = 0;
	vm.greyCount = 0;
	vm.greyStack = NULL;

	vm.bytes_allocated = 0;
	vm.next_gc = 1024 * 1024;

	table_init(&vm.globals);
	table_init(&vm.strings);
}

typedef struct Obj Obj;

static void gc_free_objects(void) {
	Obj * object = vm.objects;
	while (object != NULL) {
		Obj * next = object->next;
		gc_free_object(object);
		object = next;
	}
}

void vm_free(void) {
	table_free(&vm.globals);
	table_free(&vm.strings);
	gc_free_objects();
}

static bool is_falsey(Value value) {
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static bool call(Obj * callee, Obj_Function * function, uint8_t arg_count) {
	if (arg_count != function->arity) {
		runtime_error("expected %d arguments, but got %d", function->arity, arg_count);
		return false;
	}

	if (vm.frame_count == FRAMES_MAX) {
		runtime_error("stack overflow");
		return false;
	}

	Call_Frame * frame = &vm.frames[vm.frame_count++];
	frame->function = (Obj *)callee;
	frame->ip = function->chunk.code;

	frame->slots = vm.stack_top - arg_count - 1;

	return true;
}

typedef struct Obj_Native Obj_Native;

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

inline static bool call_function(Obj_Function * function, uint8_t arg_count) {
	return call((Obj *)function, function, arg_count);
}

inline static bool call_closure(Obj_Closure * closure, uint8_t arg_count) {
	return call((Obj *)closure, closure->function, arg_count);
}

inline static bool call_bound(Obj_Bound_Method * bound, uint8_t arg_count) {
	switch (bound->method->type) {
		case OBJ_FUNCTION:
			return call_function((Obj_Function *)bound->method, arg_count);
		default:
		// case OBJ_CLOSURE:
			return call_closure((Obj_Closure *)bound->method, arg_count);
	}
	// return false;
}

typedef struct Obj_Class Obj_Class;

static bool call_class(Obj_Class * class, uint8_t arg_count) {
	vm.stack_top[-(int32_t)(arg_count + 1)] = TO_OBJ(new_instance(class));
	return true;
}

static bool call_value(Value callee, uint8_t arg_count) {
	if (IS_OBJ(callee)) {
		switch (OBJ_TYPE(callee)) {
			case OBJ_FUNCTION:
				return call_function(AS_FUNCTION(callee), arg_count);
			case OBJ_NATIVE:
				return call_native(AS_NATIVE(callee), arg_count);
			case OBJ_CLOSURE:
				return call_closure(AS_CLOSURE(callee), arg_count);
			case OBJ_CLASS:
				return call_class(AS_CLASS(callee), arg_count);
			case OBJ_BOUND_METHOD:
				return call_bound(AS_BOUND_METHOD(callee), arg_count);
			default: break;
		}
	}

	runtime_error("can only call functions and classes");
	return false;
}

typedef struct Obj_Upvalue Obj_Upvalue;

static Obj_Upvalue * capture_upvalue(Value * local) {
	Obj_Upvalue * prev_upvalue = NULL;
	Obj_Upvalue * upvalue = vm.open_upvalues;

	while (upvalue != NULL && upvalue->location > local) {
		prev_upvalue = upvalue;
		upvalue = upvalue->next;
	}

	if (upvalue != NULL && upvalue->location == local) {
		return upvalue;
	}

	Obj_Upvalue * created_upvalue = new_upvalue(local);
	created_upvalue->next = upvalue;

	if (prev_upvalue == NULL) {
		vm.open_upvalues = created_upvalue;
	}
	else {
		prev_upvalue->next = created_upvalue;
	}

	return created_upvalue;
}

static void close_upvalues(Value * last) {
	while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
		Obj_Upvalue * upvalue = vm.open_upvalues;
		upvalue->closed = *upvalue->location;
		upvalue->location = &upvalue->closed;
		vm.open_upvalues = upvalue->next;
	}
}

typedef struct Obj_String Obj_String;
typedef struct Obj_Class Obj_Class;
typedef struct Obj_Instance Obj_Instance;

static void define_method(Obj_String * name) {
	Value method = vm_stack_peek(0);
	Obj_Class * lox_class = AS_CLASS(vm_stack_peek(1));
	table_set(&lox_class->methods, name, method);
	vm_stack_pop();
}

static bool bind_method(Obj_Class * lox_class, Obj_String * name) {
	Value method;
	if (!table_get(&lox_class->methods, name, &method)) { return false; }

	Obj_Bound_Method * bound = new_bound_method(vm_stack_peek(0), AS_OBJ(method));
	vm_stack_pop();
	vm_stack_push(TO_OBJ(bound));

	return true;
}

static Interpret_Result run(void) {
	Call_Frame * frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*(frame->ip++))
#define READ_SHORT() (frame->ip += 2, (uint16_t)(frame->ip[-2] << 8) | (uint16_t)frame->ip[-1])
#define READ_CONSTANT() (get_frame_function(frame)->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_STRING() AS_STRING(READ_CONSTANT())
#define READ_CONSTANT_FUNCTION() AS_FUNCTION(READ_CONSTANT())

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
		struct Chunk * disasseble_frame_chunk = &get_frame_function(frame)->chunk;
		chunk_disassemble_instruction(disasseble_frame_chunk, (uint32_t)(frame->ip - disasseble_frame_chunk->code));
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
				Obj_String * name = READ_CONSTANT_STRING();
				if (table_set(&vm.globals, name, vm_stack_peek(0))) {
					table_delete(&vm.globals, name);
					runtime_error("undefined variable '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				break;
			}

			case OP_GET_GLOBAL: {
				Obj_String * name = READ_CONSTANT_STRING();
				Value value;
				if (!table_get(&vm.globals, name, &value)) {
					runtime_error("undefined variable '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}
				vm_stack_push(value);
				break;
			}

			case OP_SET_UPVALUE: {
				uint8_t slot = READ_BYTE();
				Obj_Closure * frame_closure = (Obj_Closure *)frame->function;
				*frame_closure->upvalues[slot]->location = vm_stack_peek(0);
				break;
			}

			case OP_GET_UPVALUE: {
				uint8_t slot = READ_BYTE();
				Obj_Closure * frame_closure = (Obj_Closure *)frame->function;
				vm_stack_push(*frame_closure->upvalues[slot]->location);
				break;
			}

			case OP_SET_PROPERTY: {
				if (!IS_INSTANCE(vm_stack_peek(1))) {
					runtime_error("only instances have fields");
					return INTERPRET_RUNTIME_ERROR;
				}

				Obj_Instance * instance = AS_INSTANCE(vm_stack_peek(1));
				Obj_String * name = READ_CONSTANT_STRING();

				table_set(&instance->table, name, vm_stack_peek(0));

				Value value = vm_stack_pop();
				vm_stack_pop();
				vm_stack_push(value);
				break;
			}

			case OP_GET_PROPERTY: {
				if (!IS_INSTANCE(vm_stack_peek(0))) {
					runtime_error("only instances have properties");
					return INTERPRET_RUNTIME_ERROR;
				}

				Obj_Instance * instance = AS_INSTANCE(vm_stack_peek(0));
				Obj_String * name = READ_CONSTANT_STRING();

				Value value;
				if (!table_get(&instance->table, name, &value)) {
					if (bind_method(instance->lox_class, name)) { break; }
					runtime_error("undefined property '%s'", name->chars);
					return INTERPRET_RUNTIME_ERROR;
				}

				vm_stack_pop();
				vm_stack_push(value);
				break;
			}

			case OP_DEFINE_GLOBAL: {
				Obj_String * name = READ_CONSTANT_STRING();
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
					// GC protection
					Obj_String * b = AS_STRING(vm_stack_peek(0));
					Obj_String * a = AS_STRING(vm_stack_peek(1));
					Obj_String * string = strings_concatenate(a, b);
					vm_stack_push(TO_OBJ(string));
					vm_stack_pop();
					vm_stack_pop();
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

			case OP_CLOSURE: {
				Obj_Function * function = READ_CONSTANT_FUNCTION();
				Obj_Closure * closure = new_closure(function);
				vm_stack_push(TO_OBJ(closure));

				Obj_Closure * frame_closure = (Obj_Closure *)frame->function;
				for (uint32_t i = 0; i < closure->upvalue_count; i++) {
					uint8_t index = READ_BYTE();
					uint8_t is_local = READ_BYTE();
					if (is_local) {
						closure->upvalues[i] = capture_upvalue(&frame->slots[index]);
					}
					else {
						closure->upvalues[i] = frame_closure->upvalues[index];
					}
				}

				break;
			}

			case OP_CLOSE_UPVALUE: {
				close_upvalues(vm.stack_top - 1);
				vm_stack_pop();
				break;
			}

			case OP_CLASS: {
				Obj_String * name = READ_CONSTANT_STRING();
				Obj_Class * lox_class = new_class(name);
				vm_stack_push(TO_OBJ(lox_class));
				break;
			}

			case OP_METHOD: {
				Obj_String * name = READ_CONSTANT_STRING();
				define_method(name);
				break;
			}

			case OP_RETURN: {
				Value result = vm_stack_pop();

				close_upvalues(frame->slots);

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
#undef READ_CONSTANT_STRING
#undef READ_CONSTANT_FUNCTION
#undef OP_BINARY
}

typedef struct Chunk Chunk;

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
	// GC protection
	Obj_String * obj_name = copy_string(name, (uint32_t)strlen(name));
	vm_stack_push(TO_OBJ(obj_name));
	Obj_Native * obj_native = new_native(function, arity);
	vm_stack_push(TO_OBJ(obj_native));
	table_set(&vm.globals, obj_name, TO_OBJ(obj_native));
	vm_stack_pop();
	vm_stack_pop();
}
