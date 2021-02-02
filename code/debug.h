#if !defined(LOX_DEBUG)
#define LOX_DEBUG

#include "common.h"

struct Chunk;

void chunk_disassemble(struct Chunk * chunk, char const * name);
uint32_t chunk_disassemble_instruction(struct Chunk * chunk, uint32_t offset);

#endif
