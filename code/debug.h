#if !defined(LOX_DEBUG)
#define LOX_DEBUG

#include "chunk.h"

void chunk_disassemble(Chunk * chunk, char const * name);
uint32_t chunk_disassemble_instruction(Chunk * chunk, uint32_t offset);

#endif
