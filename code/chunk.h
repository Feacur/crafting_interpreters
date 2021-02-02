#if !defined(LOX_CHUNK)
#define LOX_CHUNK

#include "common.h"
#include "value.h"

typedef enum {
	OP_NIL,
	OP_CONSTANT,
	OP_FALSE,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_TRUE,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,
	OP_NEGATE,
	OP_RETURN,
} Op_Code;

struct Chunk {
	uint32_t capacity, count;
	uint8_t * code;
	uint32_t * lines;
	Value_Array constants;
};

void chunk_init(struct Chunk * chunk);
void chunk_free(struct Chunk * chunk);
void chunk_write(struct Chunk * chunk, uint8_t byte, uint32_t line);
uint32_t chunk_add_constant(struct Chunk * chunk, Value value);

#endif
