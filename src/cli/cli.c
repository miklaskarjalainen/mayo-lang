#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <stb/stb_ds.h>

#include "../common/utils.h"
#include "../common/error.h"

#include "cli.h"

#if defined(_WIN32)
#   define DEFAULT_EXECUTABLE "./output.exe"
#else
#   define DEFAULT_EXECUTABLE "./output.o"
#endif

program_params_t g_Params = { 0 };

/// arg, return true if consumed
/// params, program_params_t is the returned object from 'cli_parse' which can be modified 
typedef bool cli_func(program_params_t* params, char* arg);

static bool _exec_help(program_params_t* params, char* arg);

static bool _exec_version(program_params_t* params, char* arg) {
    UNUSED(arg);
    params->do_compilation = false;
    return false;
}

static bool _exec_echo(program_params_t* params, char* arg) {
    params->do_compilation = false;
    if (arg == NULL) {
        printf("NO ARGUMENT PASSED! :^(\n");
        return true;
    }
    printf("ECHO: %s\n", arg);
    return true;
}

static bool _exec_set_output_file(program_params_t* params, char* arg) {
    if (arg == NULL) {
        params->do_compilation = false;
        printf("NO ARGUMENT PASSED! :^(\n");
        return true;
    }
    params->output_file = arg;
    return true;
}

static const struct {
    const char* long_cmd;
    const char* short_cmd;
    const char* help;
    cli_func* func;
} s_Args[] = {
    {"--help", "-h", "lists the available commands and arguments", _exec_help},
    {"--version", "-v", "gives more details about the program", _exec_version},
    {NULL, "-o", "sets the output file", _exec_set_output_file},
    {"--echo", "-e", "prints to screen (for CLI debugging)", _exec_echo},
};

static bool _exec_help(program_params_t* params, char* arg) {
    params->do_compilation = false;
    
    UNUSED(arg);
    static const char* s_Help =
    STDOUT_YELLOW STDOUT_BOLD "Options:\n" STDOUT_RESET;

    printf("%s", s_Help);
    for (size_t i = 0; i < ARRAY_LEN(s_Args); i++) {
        // Short cmd
        printf(STDOUT_CYAN "  ");
        if (s_Args[i].short_cmd) {
            printf("%s, ", s_Args[i].short_cmd);
        }
        else {
            printf("    ");
        }

        // Long cmd
        if (s_Args[i].long_cmd) {
            printf("%s", s_Args[i].long_cmd);
        }
        printf(STDOUT_RESET);

        // equal spacing
        const int MaxCmdWidth = 12;
        const int Difference = MaxCmdWidth - (s_Args[i].long_cmd ? (int)strlen(s_Args[i].long_cmd) : 0);
        for (int j = 0; j < Difference; j++) {
            putchar(' ');
        }

        // print help
        printf(" - %s\n", s_Args[i].help);
    }

    return false;
}

program_params_t cli_parse(int argc, char** argv) {
    // default params
    program_params_t params = {
        .input_files = NULL,
        .output_file = DEFAULT_EXECUTABLE,
        .do_compilation = true,
    };

    // parse input files
    params.input_files = NULL;
    for (int i = 1; i < argc; i++) {
        const char* Cmd = argv[i];
        const unsigned int Len = strlen(Cmd);
        if (Len == 0) { continue; }

        // Input file?
        if (Cmd[0] != '-') {
            RUNTIME_ASSERT(is_file(Cmd), "could not open file at '%s'", Cmd);
            // Clone filepath
            char* file_path = calloc(Len+1, sizeof(char));
            strcpy(file_path, Cmd);
            // Store
            arrpush(params.input_files, file_path);
            continue;
        }

        // CLI argument
        bool match = false;
        const bool IsLong = Cmd[1] == '-';
        for (size_t j = 0; j < ARRAY_LEN(s_Args); j++) {
            if (IsLong) {
                if (s_Args[j].long_cmd == NULL) { continue; }
                if (strcmp(s_Args[j].long_cmd, Cmd)) {
                    continue;
                }
            }
            else {
                if (s_Args[j].short_cmd == NULL) { continue; }
                if (strcmp(s_Args[j].short_cmd, Cmd)) {
                    continue;
                }
            }

            // call the function
            RUNTIME_ASSERT(s_Args[j].func != NULL, "no function set :^(");
            if (s_Args[j].func(&params, argv[i+1])) {
                // the func consumed the argument so increment the index.
                i++;
            }
            match = true;
            break;
        }

        RUNTIME_ASSERT(match, "Could not parse command \"%s\"", Cmd);
    }

    return params;
}

void cli_delete_params(program_params_t* params) {
    if (params->input_files != NULL) {
        const size_t FileCount = arrlenu(params->input_files);
        for (size_t i = 0; i < FileCount; i++) {
            free(params->input_files[i]);
        }
        arrfree(params->input_files);
        params->input_files = NULL;
    }
}
