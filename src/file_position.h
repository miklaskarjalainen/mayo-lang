#ifndef MYLANG_FILE_POSITION
#define MYLANG_FILE_POSITION
#include <stddef.h>

typedef struct file_position_t {
    const char* filepath;
    size_t line, column;
    int length; // could be <= 0 if not needed / necessary, but maybe used for prettier error messages. 
} file_position_t;

// LEN can be <= 0 if not needed / necessary, but maybe used for prettier error messages. 
file_position_t file_pos_new(const char* fpath, size_t line, size_t column, int len);
void file_pos_print_pretty(const file_position_t* pos);

#endif
