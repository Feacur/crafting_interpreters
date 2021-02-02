#if !defined(LOX_COMPILER)
#define LOX_COMPILER

struct Chunk;

bool compile(char const * source, struct Chunk * chunk);

#endif
