#if !defined(LOX_COMMON)
#define LOX_COMMON

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#if defined(_MSC_VER ) || __STDC_VERSION__ < 199901L
	#define FLEXIBLE_ARRAY 1
#else
	#define FLEXIBLE_ARRAY
#endif

#endif
