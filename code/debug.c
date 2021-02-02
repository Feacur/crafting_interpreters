#include <stdio.h>

#include "chunk.h"
#include "debug.h"

typedef struct Chunk Chunk;

void chunk_disassemble(Chunk * chunk, char const * name) {
	printf("== %s ==\n", name);

	for (uint32_t offset = 0; offset < chunk->count;) {
		offset = chunk_disassemble_instruction(chunk, offset);
	}
}

static uint32_t constant_instruction(char const * name, Chunk * chunk, uint32_t offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '", name, constant);
	value_print(chunk->constants.values[constant]);
	printf("'\n");
	return offset + 2;
}

static uint32_t simple_instruction(char const * name, uint32_t offset) {
	printf("%s\n", name);
	return offset + 1;
}

uint32_t chunk_disassemble_instruction(Chunk * chunk, uint32_t offset) {
	printf("%04d ", offset);
	if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
		printf("   | ");
	}
	else {
		printf("%04d ", chunk->lines[offset]);
	}

	uint8_t instruction = chunk->code[offset];
	switch (instruction) {
		case OP_CONSTANT:
			return constant_instruction("OP_CONSTANT", chunk, offset);
		case OP_NIL:
			return simple_instruction("OP_NIL", offset);
		case OP_FALSE:
			return simple_instruction("OP_FALSE", offset);
		case OP_EQUAL:
			return simple_instruction("OP_EQUAL", offset);
		case OP_GREATER:
			return simple_instruction("OP_GREATER", offset);
		case OP_LESS:
			return simple_instruction("OP_LESS", offset);
		case OP_TRUE:
			return simple_instruction("OP_TRUE", offset);
		case OP_ADD:
			return simple_instruction("OP_ADD", offset);
		case OP_SUBTRACT:
			return simple_instruction("OP_SUBTRACT", offset);
		case OP_MULTIPLY:
			return simple_instruction("OP_MULTIPLY", offset);
		case OP_DIVIDE:
			return simple_instruction("OP_DIVIDE", offset);
		case OP_NOT:
			return simple_instruction("OP_NOT", offset);
		case OP_NEGATE:
			return simple_instruction("OP_NEGATE", offset);
		case OP_RETURN:
			return simple_instruction("OP_RETURN", offset);
		default:
			printf("unknown opcode %d\n", instruction);
			return offset + 1;
	}
}
