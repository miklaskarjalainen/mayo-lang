#ifndef MYLANG_PARSER_ERROR_H
#define MYLANG_PARSER_ERROR_H

#define PARSER_ERROR(pos, ...) \
    PRINT_ERROR_IN_FILE(pos, __VA_ARGS__); \
    exit(-1)

#define PARSER_WARNING(pos, ...) \
    PRINT_ERROR_IN_FILE(pos, __VA_ARGS__)

#define PARSER_ASSERT(expr, pos, ...)               \
    do {                                            \
        if (!(expr)) {                              \
            PARSER_ERROR(pos, __VA_ARGS__);         \
        }                                           \
    } while(0)

#endif
