#pragma once
#include <stddef.h>
#include "common/utils.h"

#ifdef __GNUC__
    #define __FORMAT_ATTRIB__ __attribute__ ((format (printf, 5, 6)))
#else
    #define __FORMAT_ATTRIB__
#endif

// Basic printing of a file
void print_cat(const char* contents);

//* 'fpath' filepath for file
//* 'line' which the error is in
//* 'column' which the error is in
//* 'word_len' how long the error is (in characters).
//* 'reason' the reason to be printed (can be NULL).
void print_error_in_file(const char* fpath, size_t line, size_t column, int word_len, const char* format, ...) __FORMAT_ATTRIB__;
//* file_position_t for the pos
#define PRINT_ERROR_IN_FILE(pos, ...)                                                               \
    do {                                                                                            \
        printf(__FILE__ ":%i:\n" STDOUT_RESET, __LINE__);                                           \
        print_error_in_file((pos).filepath, (pos).line, (pos).column, (pos).length, __VA_ARGS__);   \
    } while(0)
