#if !defined(LOX_COMMON)
#define LOX_COMMON

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// #define DEBUG_GC_STRESS
// #define DEBUG_PRINT_BYTECODE
// #define DEBUG_TRACE_GC
// #define DEBUG_TRACE_EXECUTION

#define NAN_BOXING
#define NAN_MASK ((uint64_t)0x7ffc000000000000)
#define NAN_SIGN ((uint64_t)0x8000000000000000)
#define NAN_TAG_NIL   1
#define NAN_TAG_FALSE 2
#define NAN_TAG_TRUE  3

#define FRAMES_MAX 64
#define LOCALS_MAX (UINT8_MAX + 1)
#define STACK_MAX (FRAMES_MAX * LOCALS_MAX)

// -- flexible array member settings
#if __STDC_VERSION__ >= 199901L
	#if defined(__clang__)
		#define FLEXIBLE_ARRAY
	#endif
#endif // __STDC_VERSION__

#if !defined(FLEXIBLE_ARRAY)
	#define FLEXIBLE_ARRAY 1
#endif

// -- debug settings
#if defined(LOX_TARGET_DEBUG)
	#if defined(__clang__)
		#define DEBUG_BREAK() __builtin_debugtrap()
	#elif defined(_MSC_VER)
		#define DEBUG_BREAK() __debugbreak()
	#endif
#endif // LOX_TARGET_DEBUG

#if !defined(DEBUG_BREAK)
	#define DEBUG_BREAK() (void)0
#endif

#endif
