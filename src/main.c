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
        lexer_lex(&lexer);
        const size_t TkCount = arrlenu(lexer.tokens);
        for (size_t tk_idx = 0; tk_idx < TkCount; tk_idx++) {
            token_print_pretty(&lexer.tokens[tk_idx]);
        }

        // Parsing
        parser_parse(&parser);
        print_ast_tree(parser.node_root);
        semantic_analysis(parser.node_root);
    }

    parser_cleanup(&parser);
    lexer_cleanup(&lexer); 
    arena_free(&arena);
clean_params:;
    cli_delete_params(&g_Params);
    

    return exit_code;
} 


