#ifndef MY_LANG_ERROR_H
#define MY_LANG_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "utils.h"

extern jmp_buf g_Jumpluff;

#define PANIC_EXIT_CODE EXIT_FAILURE
#define ASSERT_EXIT_CODE EXIT_FAILURE

#define SETJUMP() (setjmp(g_Jumpluff))
#define LONGJUMP(num) longjmp(g_Jumpluff, num)

#define PANIC(...)                                                                                      \
    do {                                                                                                \
        printf(STDOUT_ERROR "Error: " STDOUT_RESET "program panicked at " __FILE__ ":%i '", __LINE__);  \
        printf(__VA_ARGS__);                                                                            \
        printf("'\n");                                                                                  \
        LONGJUMP(EXIT_FAILURE);                                                                         \
    } while(0)

// Is not removed in release builds, see 'DEBUG_ASSERT'.
#define RUNTIME_ASSERT(expr, ...)                                                                           \
    do {                                                                                                    \
        if (!(expr)) {                                                                                      \
            printf(STDOUT_ERROR "Error: " STDOUT_RESET "assertion failed at " __FILE__ ":%i '", __LINE__);  \
            printf(__VA_ARGS__);                                                                            \
            printf("'\n");                                                                                  \
            LONGJUMP(EXIT_FAILURE);                                                                         \
        }                                                                                                   \
    } while(0)

// Same as assert, but is removed in release builds.
#if __OPTIMIZE__
#define DEBUG_ASSERT(expr, ...) do {UNUSED(expr);} while (0)
#else
#define DEBUG_ASSERT(expr, ...) RUNTIME_ASSERT(expr, __VA_ARGS__)
#endif

#define TODO(msg) PANIC("TODO: " msg)
#define UNIMPLEMENTED(msg) PANIC("Unimplemented: " msg)

#endif
