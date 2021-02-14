#if !defined(LOX_COMPILER)
#define LOX_COMPILER

struct Obj_Function;
struct Obj_Function * compile(char const * source);

void gc_mark_compiler_roots_grey(void);

#endif
