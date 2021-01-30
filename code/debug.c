#include <stdio.h>

#include "debug.h"

void chunk_disassemble(Chunk * chunk, char const * name) {
	printf("== %s ==\n", name);

	for (uint32_t offset = 0; offset < chunk->count;) {
		offset = chunk_disassemble_instruction(chunk, offset);
	}
}

static uint32_t constant_instruction(char const * name, Chunk * chunk, uint32_t offset) {
	uint8_t constant = chunk->code[offset + 1];
	printf("%-16s %4d '", name, constant);
	print_value(chunk->constants.values[constant]);
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
		case OP_RETURN:
			return simple_instruction("OP_RETURN", offset);
		default:
			printf("unknown opcode %d\n", instruction);
			return offset + 1;
	}
}
