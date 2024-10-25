#include <stdio.h>
#include "cli/cli.h"

#include "common/error.h"
#include "common/string.h"
#include "common/sym_table.h"
#include "common/stats.h"
#include "common/utils.h"

#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "parser/ast_type.h"
#include "parser/ast_print.h"
#include "backend_qbe.h"
#include "optimizer/optimize.h"
#include "string.h"

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

jmp_buf g_Jumpluff;

// Use "optimal" size for debug builds. Just so we can test more of our arena implementation.
#ifdef NDEBUG
#define ARENA_CAPACITY 4096
#else
#define ARENA_CAPACITY sizeof(ast_node_t) * 2
#endif


int main(int argc, char** argv) {
    clock_t arg_parse_duration = 0;
    clock_t lex_duration = 0;
    clock_t parse_duration = 0;
    clock_t analysis_duration = 0;
    clock_t qbe_gen_duration = 0;

    PERF_BEGIN(ProgramBegin);
    
    int exit_code = SETJUMP();
    if (!exit_code) {
        g_Params = cli_parse(argc, argv);
        if (!g_Params.do_compilation) {
            goto clean_params;
        }
    }
    else {
        goto clean_params;
    }
    arg_parse_duration = PERF_END(ProgramBegin);

    // Get path 
    const size_t InputFileCount = arrlenu(g_Params.input_files);
    RUNTIME_ASSERT(InputFileCount > 0, "no input files :^(");
    const char* Path = g_Params.input_files[0];

    // Initialize lexer & parser
    arena_t arena;
    arena_init(&arena, ARENA_CAPACITY);

    lexer_t lexer = { 0 };
    lexer_init(&lexer, &arena, Path);
    parser_t parser = parser_new(&arena, &lexer);

    exit_code = SETJUMP();
    if (!exit_code) {
        // Lex
        PERF_BEGIN(LexBegin);
        lexer_lex(&lexer);
        if (g_Params.print_tokens) {
            const size_t TkCount = arrlenu(lexer.tokens);
            for (size_t tk_idx = 0; tk_idx < TkCount; tk_idx++) {
                token_print_pretty(&lexer.tokens[tk_idx]);
            }
        }
        lex_duration = PERF_END(LexBegin);

        // Parsing
        PERF_BEGIN(ParseBegin);
        parser_parse(&parser);
        parse_duration = PERF_END(ParseBegin);

        PERF_BEGIN(AnalysisBegin);
        semantic_analysis(&arena, parser.node_root);
        perform_ast_optimizations(parser.node_root);
        analysis_duration = PERF_END(AnalysisBegin);

        if (g_Params.print_ast) {
            print_ast_tree(parser.node_root);
        }

        // Output qbe
        PERF_BEGIN(QbeBegin);
        FILE* f = fopen("output.ssa", "w");
        generate_qbe(f, parser.node_root);
        fclose(f);
        qbe_gen_duration = PERF_END(QbeBegin);

        CMD("qbe output.ssa -o output.s");

        // generate command to gcc
        {
            const char* CmdTemplate = "gcc -o output.o output.s ";
            const size_t DesiredLen = strlen(g_Params.cflags) + strlen(CmdTemplate) + 1;
            
            // assemble
            char* cmd = malloc(DesiredLen);
            strcpy(cmd, CmdTemplate);
            strcat(cmd, g_Params.cflags);
            CMD(cmd);
            free(cmd);
        }

        CMD("./output.o");
    }
    
    parser_cleanup(&parser);
    lexer_cleanup(&lexer); 
    arena_free(&arena);
clean_params:;
    cli_delete_params(&g_Params);
    
    clock_t ProgramDuration = PERF_END(ProgramBegin);

    // Print performance
    printf("PERFORMANCE:\n");
    printf("  Arg parse duration: ");     PRINT_DURATION(arg_parse_duration); printf("\n");
    printf("  Code lex duration: ");      PRINT_DURATION(lex_duration); printf("\n");
    printf("  Code parse duration: ");    PRINT_DURATION(parse_duration); printf("\n");
    printf("  Code analysis duration: "); PRINT_DURATION(analysis_duration); printf("\n");
    printf("  Qbe Generate duration: ");       PRINT_DURATION(qbe_gen_duration); printf("\n");
    printf("  Program duration: ");       PRINT_DURATION(ProgramDuration); printf("\n");


    return exit_code;
} 


