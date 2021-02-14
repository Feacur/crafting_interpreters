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

static void gc_mark_roots_grey(void) {
	for (Value * slot = vm.stack; slot < vm.stack_top; slot++) {
		gc_mark_value_grey(*slot);
	}

	for (uint32_t i = 0; i < vm.frame_count; i++) {
		gc_mark_object_grey(vm.frames[i].function);
	}

	for (Obj_Upvalue * upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
		gc_mark_object_grey((Obj *)upvalue);
	}

	// `vm.strings` is a weak-references root
	gc_mark_table_grey(&vm.globals);
}

static void gc_grey_to_black(void) {
	while (vm.greyCount > 0) {
		Obj * object = vm.greyStack[--vm.greyCount];
		gc_mark_object_black(object);
	}
}

static void gc_sweep_white(void) {
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

	gc_mark_roots_grey();
	gc_mark_compiler_roots_grey();
	gc_grey_to_black();
	gc_table_remove_white_keys(&vm.strings);
	gc_sweep_white();

#if defined(DEBUG_TRACE_GC)
	printf("-- gc end\n");
	if (bytes_before > vm.bytes_allocated) {
		printf("   collected %zu bytes (from %zu to %zu); next at: %zu\n", bytes_before - vm.bytes_allocated, bytes_before, vm.bytes_allocated, vm.next_gc);
	}
#endif // DEBUG_TRACE_GC
}
