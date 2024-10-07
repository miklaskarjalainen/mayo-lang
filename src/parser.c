#include <stb/stb_ds.h>
#include <stdio.h>

#include "common/error.h"
#include "common/utils.h"
#include "lexer/lexer_token.h"
#include "parser/ast_type.h"
#include "parser/parser_parse.h"

#include "lexer.h"
#include "parser.h"

parser_t parser_new(lexer_t* lexer) {
    RUNTIME_ASSERT(lexer != NULL, "lexer is null");

    ast_node_t* root = calloc(1, sizeof(ast_node_t));
    RUNTIME_ASSERT(root, "root is NULL");
    root->kind = AST_TRANSLATION_UNIT;
    root->data.translation_unit.body = NULL;

    return (parser_t){
        .node_root = root,
        .token_index = 0,
        .lexer = lexer
    };
}

void parser_cleanup(parser_t* parser) {
    ast_free_tree(parser->node_root);
}

void parser_parse(parser_t* parser) {
    printf("PARSER: \n");
    RUNTIME_ASSERT(parser->node_root, "root is NULL");
    parser->node_root->data.translation_unit.body = parse_global_scope(parser);
}


