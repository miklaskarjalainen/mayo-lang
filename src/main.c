#include <stdio.h>
#include "common/error.h"
#include "common/string.h"
#include "common/utils.h"
#include "cli/cli.h"

#include "lexer.h"
#include "parser.h"
#include "parser/ast_type.h"
#include "parser/ast_print.h"
#include "typechecker/typechecker.h"

#define STB_DS_IMPLEMENTATION
#include <stb/stb_ds.h>

int main(int argc, char** argv) {
    g_Params = cli_parse(argc, argv);

    if (g_Params.do_compilation) {
        const size_t InputFileCount = arrlenu(g_Params.input_files);

        RUNTIME_ASSERT(InputFileCount > 0, "no input files :^(");

        for (size_t file_idx = 0; file_idx < InputFileCount; file_idx++) {
            const char* Path = g_Params.input_files[file_idx];

            // Lexing
            lexer_t* lexer = lexer_new(Path);
            RUNTIME_ASSERT(lexer != NULL, "Could not create lexer! :^(");
            lexer_lex(lexer);
            {
                const size_t TkCount = arrlenu(lexer->tokens);
                for (size_t tk_idx = 0; tk_idx < TkCount; tk_idx++) {
                    token_print_pretty(&lexer->tokens[tk_idx]);
                }
            }
            
        
            // Parsing
            parser_t parser = parser_new(lexer);
            parser_parse(&parser);
            print_ast_tree(parser.node_root);

            // clean up
            parser_cleanup(&parser);
            lexer_delete(lexer); 
        }
    }

    cli_delete_params(&g_Params);
    
    return 0;
} 


