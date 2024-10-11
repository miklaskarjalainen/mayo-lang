#include <stdio.h>
#include "cli/cli.h"

#include "common/error.h"
#include "common/string.h"
#include "common/sym_table.h"
#include "common/utils.h"

#include "lexer.h"
#include "parser.h"
#include "semantics.h"
#include "parser/ast_type.h"
#include "parser/ast_print.h"

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

jmp_buf g_Jumpluff;

int main(int argc, char** argv) {
    // To simplify to code flow here, lexer should able to be constructed without a path, and make it allocated on stack?
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
    lexer_t* lexer = lexer_new(Path);
    RUNTIME_ASSERT(lexer != NULL, "Could not create lexer! :^(");
    parser_t parser = parser_new(lexer);

    exit_code = SETJUMP();
    if (!exit_code) {
        // Lex
        lexer_lex(lexer);
        const size_t TkCount = arrlenu(lexer->tokens);
        for (size_t tk_idx = 0; tk_idx < TkCount; tk_idx++) {
            token_print_pretty(&lexer->tokens[tk_idx]);
        }

        // Parsing
        parser_parse(&parser);
        print_ast_tree(parser.node_root);
        semantic_analysis(parser.node_root);
    }

    parser_cleanup(&parser);
    lexer_delete(lexer); 
clean_params:;
    cli_delete_params(&g_Params);
    
    return exit_code;
} 


