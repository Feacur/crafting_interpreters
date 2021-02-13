#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "compiler.h"
#include "vm.h"

#if defined(DEBUG_GC_LOG)
#include <stdio.h>
#include "debug.h"
#endif

typedef struct Obj Obj;

void * reallocate(void * pointer, size_t old_size, size_t new_size) {
	if (new_size > old_size) {
#ifdef DEBUG_GC_STRESS
		collect_garbage();
#endif
	}

	if (new_size == 0) {
		free(pointer);
		return NULL;
	}

	void * result = realloc(pointer, new_size);
	if (result == NULL) { exit(1); }
	return result;
}

void objects_free(void) {
	Obj * object = vm.objects;
	while (object != NULL) {
		Obj * next = object->next;
		object_free(object);
		object = next;
	}
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

void collect_garbage(void) {
#if defined(DEBUG_GC_LOG)
	printf("-- gc begin\n");
#endif

	gc_mark_roots();
	gc_mark_compiler_roots();

#if defined(DEBUG_GC_LOG)
	printf("-- gc end\n");
#endif
}
