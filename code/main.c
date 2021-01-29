#include <stdio.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;

	Chunk chunk;
	chunk_init(&chunk);

	uint32_t constant = chunk_add_constant(&chunk, 1.2);
	chunk_write(&chunk, OP_CONSTANT, 123);
	chunk_write(&chunk, (uint8_t)constant, 123);

	chunk_write(&chunk, OP_RETURN, 123);

	chunk_disassemble(&chunk, "test chunk");
	chunk_free(&chunk);
	(void)getchar();
	return 0;
}
