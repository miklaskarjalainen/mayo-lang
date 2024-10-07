#include <stdio.h>
#include "file_position.h"

file_position_t file_pos_new(const char* fpath, size_t line, size_t column, int len) {
    return (file_position_t) {
        .filepath = fpath,
        .line = line,
        .column = column,
        .length = len
    };
}

void file_pos_print_pretty(const file_position_t* pos) {
    printf(
        "file_position_t {\n"
        "    filepath = \"%s\",\n"
        "    line = %zu,\n"
        "    column = %zu,\n"
        "    length = %i\n"
        "};\n",
        pos->filepath,
        pos->line,
        pos->column,
        pos->length
    );
}
