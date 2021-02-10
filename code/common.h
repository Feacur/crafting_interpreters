#if !defined(LOX_COMMON)
#define LOX_COMMON

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION


#define FRAMES_MAX 64
#define LOCALS_MAX (UINT8_MAX + 1)
#define STACK_MAX (FRAMES_MAX * LOCALS_MAX)

#if defined(_MSC_VER ) || __STDC_VERSION__ < 199901L
	#define FLEXIBLE_ARRAY 1
#else
	#define FLEXIBLE_ARRAY
#endif

#endif
