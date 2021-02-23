#if !defined(LOX_MEMORY)
#define LOX_MEMORY

#include "common.h"

#define GROWTH_FACTOR 2

#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * GROWTH_FACTOR)

#define GROW_ARRAY(pointer, old_count, new_count) \
	reallocate(pointer, sizeof(*pointer) * (old_count), sizeof(*pointer) * (new_count))

#define FREE_ARRAY(pointer, old_count) \
	reallocate(pointer, sizeof(*pointer) * (old_count), 0)

void * reallocate(void * pointer, size_t old_size, size_t new_size);

void gc_run(void);

#endif
