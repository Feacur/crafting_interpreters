#if !defined(LOX_CHUNK)
#define LOX_CHUNK

#include "common.h"
#include "value.h"

typedef enum {
	OP_CONSTANT,
	OP_RETURN,
} Op_Code;

typedef struct {
	uint32_t capacity, count;
	uint8_t * code;
	uint32_t * lines;
	Value_Array constants;
} Chunk;

void chunk_init(Chunk * chunk);
void chunk_free(Chunk * chunk);
void chunk_write(Chunk * chunk, uint8_t byte, uint32_t line);
uint32_t chunk_add_constant(Chunk * chunk, Value value);

#endif
