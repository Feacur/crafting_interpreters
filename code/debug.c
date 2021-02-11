#include <stdio.h>

#include "object.h"
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

static uint32_t byte_instruction(char const * name, Chunk * chunk, uint32_t offset) {
	uint8_t slot = chunk->code[offset + 1];
	printf("%-16s %4d\n", name, slot);
	return offset + 2;
}

static uint32_t jump_instruction(char const * name, int32_t sign, Chunk * chunk, uint32_t offset) {
	uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8) | (uint16_t)(chunk->code[offset + 2]);
	printf("%-16s %4d -> %d\n", name, offset, (int32_t)(offset + 3) + sign * (int32_t)jump);
	return offset + 3;
}

static uint32_t simple_instruction(char const * name, uint32_t offset) {
	printf("%s\n", name);
	return offset + 1;
}

typedef struct Obj_Function Obj_Function;

uint32_t chunk_disassemble_instruction(Chunk * chunk, uint32_t offset) {
	printf("%04d ", offset);
	if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
		printf("   | ");
	}
	else {
		printf("%04d ", chunk->lines[offset]);
	}

	Op_Code instruction = chunk->code[offset];
	switch (instruction) {
		case OP_CONSTANT:
			return constant_instruction("OP_CONSTANT", chunk, offset);
		case OP_SET_LOCAL:
			return byte_instruction("OP_SET_LOCAL", chunk, offset);
		case OP_GET_LOCAL:
			return byte_instruction("OP_GET_LOCAL", chunk, offset);
		case OP_SET_UPVALUE:
			return byte_instruction("OP_SET_UPVALUE", chunk, offset);
		case OP_GET_UPVALUE:
			return byte_instruction("OP_GET_UPVALUE", chunk, offset);
		case OP_CALL:
			return byte_instruction("OP_CALL", chunk, offset);
		case OP_SET_GLOBAL:
			return constant_instruction("OP_SET_GLOBAL", chunk, offset);
		case OP_GET_GLOBAL:
			return constant_instruction("OP_GET_GLOBAL", chunk, offset);
		case OP_DEFINE_GLOBAL:
			return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
		case OP_NIL:
			return simple_instruction("OP_NIL", offset);
		case OP_FALSE:
			return simple_instruction("OP_FALSE", offset);
		case OP_POP:
			return simple_instruction("OP_POP", offset);
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
		case OP_LOOP:
			return jump_instruction("OP_LOOP", -1, chunk, offset);
		case OP_JUMP:
			return jump_instruction("OP_JUMP", 1, chunk, offset);
		case OP_JUMP_IF_FALSE:
			return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
		case OP_CLOSURE: {
			uint8_t constant = chunk->code[offset + 1];
			printf("%-16s %4d '", "OP_CLOSURE", constant);
			value_print(chunk->constants.values[constant]);
			printf("'\n");

			Obj_Function * function = AS_FUNCTION(chunk->constants.values[constant]);
			for (uint32_t i = 0; i < function->upvalue_count; i++) {
				uint8_t index = chunk->code[offset + 2 + i * 2];
				uint8_t is_local = chunk->code[offset + 3 + i * 2];
				printf(
					"%04d      |                     %s %d\n",
					offset + i * 2, is_local ? "local" : "upvalue", index
				);
			}

			return offset + 2 + function->upvalue_count * 2;
		}
		case OP_RETURN:
			return simple_instruction("OP_RETURN", offset);
	}

	printf("unknown opcode %d\n", instruction);
	return offset + 1;
}
