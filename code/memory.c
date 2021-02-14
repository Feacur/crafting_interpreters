#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "compiler.h"
#include "vm.h"

#if defined(DEBUG_TRACE_GC)
#include <stdio.h>
#endif // DEBUG_TRACE_GC

#define GC_HEAP_GROW_FACTOR 2

typedef struct Obj Obj;

void * reallocate(void * pointer, size_t old_size, size_t new_size) {
	vm.bytes_allocated += new_size - old_size;

#ifdef DEBUG_GC_STRESS
	if (new_size > old_size) {
		gc_run();
	}
#else
	if (vm.bytes_allocated > vm.next_gc) {
		gc_run();
	}
#endif // DEBUG_GC_STRESS

	if (new_size == 0) {
		free(pointer);
		return NULL;
	}

	void * result = realloc(pointer, new_size);
	if (result == NULL) { exit(1); }
	return result;
}

typedef struct Obj_Upvalue Obj_Upvalue;

static void gc_mark_roots(void) {
	for (Value * slot = vm.stack; slot < vm.stack_top; slot++) {
		gc_mark_value(*slot);
	}

	for (uint32_t i = 0; i < vm.frame_count; i++) {
		gc_mark_object(vm.frames[i].function);
	}

	for (Obj_Upvalue * upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
		gc_mark_object((Obj *)upvalue);
	}

	gc_mark_table(&vm.globals);
}

typedef struct Obj_Function Obj_Function;
typedef struct Obj_Closure Obj_Closure;

static void object_mark_black(Obj * object) {
#if defined(DEBUG_TRACE_GC)
	printf("%p mark black ", (void *)object);
	print_object((Obj *)object);
	printf("\n");
#endif

	switch (object->type) {
		case OBJ_STRING:
		case OBJ_NATIVE:
			break;

		case OBJ_UPVALUE: {
			gc_mark_value(((Obj_Upvalue *)object)->closed);
			break;
		}

		case OBJ_FUNCTION: {
			Obj_Function * function = (Obj_Function *)object;
			gc_mark_object((Obj *)function->name);
			gc_mark_value_array(&function->chunk.constants);
			break;
		}

		case OBJ_CLOSURE: {
			Obj_Closure * closure = (Obj_Closure *)object;
			gc_mark_object((Obj *)closure->function);
			for (uint32_t i = 0; i < closure->upvalue_count; i++) {
				gc_mark_object((Obj *)closure->upvalues[i]);
			}
			break;
		}
	}
}

static void trace_references(void) {
	while (vm.greyCount > 0) {
		Obj * object = vm.greyStack[--vm.greyCount];
		object_mark_black(object);
	}
}

static void gc_sweep(void) {
	Obj * previous = NULL;
	Obj * object = vm.objects;

	while (object != NULL) {
		if (object->is_marked) {
			object->is_marked = false;
			previous = object;
			object = object->next;
		}
		else {
			Obj * unreached = object;

			object = object->next;
			if (previous == NULL) {
				vm.objects = object;
			}
			else {
				previous->next = object;
			}

			gc_free_object(unreached);
		}
	}

	vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;
}

void gc_run(void) {
#if defined(DEBUG_TRACE_GC)
	size_t bytes_before = vm.bytes_allocated;
	printf("-- gc begin\n");
#endif // DEBUG_TRACE_GC

	gc_mark_roots();
	gc_mark_compiler_roots();
	trace_references();
	table_remove_white_keys(&vm.strings);
	gc_sweep();

#if defined(DEBUG_TRACE_GC)
	printf("-- gc end\n");
	if (bytes_before > vm.bytes_allocated) {
		printf("   collected %zu bytes (from %zu to %zu); next at: %zu\n", bytes_before - vm.bytes_allocated, bytes_before, vm.bytes_allocated, vm.next_gc);
	}
#endif // DEBUG_TRACE_GC
}
