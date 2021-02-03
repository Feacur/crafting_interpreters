#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "vm.h"

typedef struct Obj Obj;

void * reallocate(void * pointer, size_t old_size, size_t new_size) {
	(void)old_size;
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
