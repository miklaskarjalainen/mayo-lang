#ifndef MYLANG_CLI_H
#define MYLANG_CLI_H
#include <stdbool.h>

typedef struct program_params_t {
    const char* output_file;
    char** input_files;
    char* cflags;
    bool do_compilation;
} program_params_t;

extern program_params_t g_Params;

program_params_t cli_parse(int argc, char** argv);
void cli_delete_params(program_params_t* params);

#endif
