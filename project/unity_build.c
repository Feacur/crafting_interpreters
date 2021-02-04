#include "code/memory.c"
#include "code/value.c"
#include "code/object.c"
#include "code/table.c"
#include "code/chunk.c"
#include "code/scanner.c"
#include "code/compiler.c"
#include "code/vm.c"
#include "code/main.c"

#if defined(DEBUG_TRACE_EXECUTION) || defined(DEBUG_PRINT_CODE)
#include "code/debug.c"
#endif // DEBUG_TRACE_EXECUTION, DEBUG_PRINT_CODE
