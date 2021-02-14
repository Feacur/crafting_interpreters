#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, flexible, object_type) \
	(type *)(void *)allocate_object(sizeof(type) + flexible, object_type)

#define FREE_OBJ(pointer, flexible) \
	reallocate(pointer, sizeof(*pointer) + flexible, 0)

typedef struct VM VM;

typedef struct Obj Obj;

static Obj * allocate_object(size_t size, Obj_Type type) {
	Obj * object = (Obj *)reallocate(NULL, 0, size);
#if defined(DEBUG_TRACE_GC)
	printf("%p allocate %zu, type %d\n", (void *)object, size, type);
#endif // DEBUG_TRACE_GC

	object->type = type;
	object->is_marked = false;

	object->next = vm.objects;
	vm.objects = object;

	return object;
}

typedef struct Obj_String Obj_String;

static Obj_String * allocate_string(uint32_t length) {
	Obj_String * string = ALLOCATE_OBJ(Obj_String, sizeof(char) * (length + 1), OBJ_STRING);
	string->length = length;
	string->chars[length] = '\0';
	return string;
}

inline static uint32_t hash_string_2(uint32_t hash, char const * chars, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
		hash ^= (uint8_t)chars[i];
		hash *= 16777619u;
	}
	return hash;
}

static uint32_t hash_string(char const * chars, uint32_t length) {
	// FNV-1a
	// http://www.isthe.com/chongo/tech/comp/fnv/
	// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
	return hash_string_2(2166136261u, chars, length);
}

Obj_String * copy_string(char const * chars, uint32_t length) {
	uint32_t hash = hash_string(chars, length);
	Obj_String * interned = table_find_key(&vm.strings, chars, length, hash);
	if (interned != NULL) { return interned; }

	Obj_String * string = allocate_string(length);
	memcpy(string->chars, chars, length);
	string->hash = hash;

	// GC protection
	vm_stack_push(TO_OBJ(string));
	table_set(&vm.strings, string, TO_NIL());
	vm_stack_pop();

	return string;
}

typedef struct Obj_Function Obj_Function;
typedef struct Obj_Native Obj_Native;
typedef struct Obj_Closure Obj_Closure;
typedef struct Obj_Upvalue Obj_Upvalue;
typedef struct Obj_Class Obj_Class;

void print_object(Obj * object) {
	switch (object->type) {
		case OBJ_STRING:
			Obj_String * string = (Obj_String *)object;
			printf("%s", string->chars);
			break;

		case OBJ_FUNCTION: {
			Obj_Function * function = (Obj_Function *)object;
			if (function->name == NULL) {
				printf("<script>");
			}
			else {
				printf("<fn %s>", function->name->chars);
			}
			break;
		}

		case OBJ_NATIVE: {
			// Obj_Native * native = (Obj_Native *)object;
			printf("<native fn>");
			break;
		}

		case OBJ_CLOSURE: {
			Obj_Closure * closure = (Obj_Closure *)object;
			print_object((Obj *)closure->function);
			break;
		}

		case OBJ_UPVALUE: {
			// Obj_Upvalue * upvalue = (Obj_Upvalue *)object;
			// value_print(*upvalue->location);
			printf("upvalue");
			break;
		}

		case OBJ_CLASS: {
			Obj_Class * lox_class = (Obj_Class *)object;
			printf("%s", lox_class->name->chars);
			break;
		}
	}
}

Obj_String * strings_concatenate(Obj_String * a_string, Obj_String * b_string) {
	uint32_t hash = hash_string_2(a_string->hash, b_string->chars, b_string->length);
	uint32_t length = a_string->length + b_string->length;
	Obj_String * interned = table_find_key_2(&vm.strings, a_string->chars, a_string->length, b_string->chars, b_string->length, hash);
	if (interned != NULL) { return interned; }

	Obj_String * string = allocate_string(length);
	memcpy(string->chars, a_string->chars, sizeof(char) * a_string->length);
	memcpy(string->chars + a_string->length, b_string->chars, sizeof(char) * b_string->length);
	string->hash = hash;

	// GC protection
	vm_stack_push(TO_OBJ(string));
	table_set(&vm.strings, string, TO_NIL());
	vm_stack_pop();

	return string;
}

Obj_Function * new_function(void) {
	Obj_Function * function = ALLOCATE_OBJ(Obj_Function, 0, OBJ_FUNCTION);
	function->arity = 0;
	function->upvalue_count = 0;
	function->name = NULL;
	chunk_init(&function->chunk);
	return function;
}

Obj_Native * new_native(Native_Fn * function, uint8_t arity) {
	Obj_Native * native = ALLOCATE_OBJ(Obj_Native, 0, OBJ_NATIVE);
	native->function = function;
	native->arity = arity;
	return native;
}

Obj_Closure * new_closure(Obj_Function * function) {
	Obj_Upvalue ** upvalues = reallocate(NULL, 0, sizeof(Obj_Upvalue *) * function->upvalue_count);
	for (uint32_t i = 0; i < function->upvalue_count; i++) {
		upvalues[i] = NULL;
	}

	Obj_Closure * closure = ALLOCATE_OBJ(Obj_Closure, 0, OBJ_CLOSURE);
	closure->function = function;
	closure->upvalues = upvalues;
	closure->upvalue_count = function->upvalue_count;

	return closure;
}

Obj_Upvalue * new_upvalue(Value * slot) {
	Obj_Upvalue * upvalue = ALLOCATE_OBJ(Obj_Upvalue, 0, OBJ_UPVALUE);
	upvalue->closed = TO_NIL();
	upvalue->location = slot;
	upvalue->next = NULL;
	return upvalue;
}

Obj_Class * new_class(Obj_String * name) {
	Obj_Class * lox_class = ALLOCATE_OBJ(Obj_Class, 0, OBJ_CLASS);
	lox_class->name = name;
	return lox_class;
}

void gc_free_object(Obj * object) {
#if defined(DEBUG_TRACE_GC)
	printf("%p free, type %d\n", (void *)object, object->type);
#endif // DEBUG_TRACE_GC

	switch (object->type) {
		case OBJ_STRING: {
			Obj_String * string = (Obj_String *)object;
			FREE_OBJ(string, sizeof(char) * string->length);
			break;
		}

		case OBJ_FUNCTION: {
			Obj_Function * function = (Obj_Function *)object;
			chunk_free(&function->chunk);
			FREE_OBJ(function, 0);
			break;
		}

		case OBJ_NATIVE: {
			Obj_Native * native = (Obj_Native *)object;
			FREE_OBJ(native, 0);
			break;
		}

		case OBJ_CLOSURE: {
			Obj_Closure * closure = (Obj_Closure *)object;
			reallocate(closure->upvalues, sizeof(*closure->upvalues) * closure->upvalue_count, 0);
			FREE_OBJ(closure, 0);
			break;
		}

		case OBJ_UPVALUE: {
			Obj_Upvalue * upvalue = (Obj_Upvalue *)object;
			FREE_OBJ(upvalue, 0);
			break;
		}

		case OBJ_CLASS: {
			Obj_Class * lox_class = (Obj_Class *)object;
			FREE_OBJ(lox_class, 0);
			break;
		}
	}
}

void gc_mark_object(Obj * object) {
	if (object == NULL) { return; }
	if (object->is_marked) { return; }

#if defined(DEBUG_TRACE_GC)
	printf("%p mark ", (void *)object);
	print_object(object);
	printf("\n");
#endif // DEBUG_TRACE_GC

	object->is_marked = true;

	if (vm.greyCapacity < vm.greyCount + 1) {
		vm.greyCapacity = GROW_CAPACITY(vm.greyCapacity);
		vm.greyStack = realloc(vm.greyStack, sizeof(*vm.greyStack) * vm.greyCapacity);
		if (vm.greyStack == NULL) { exit(1); }
	}

	vm.greyStack[vm.greyCount++] = object;
}
