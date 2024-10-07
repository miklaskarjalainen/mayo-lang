#include <stb/stb_ds.h>
#include <stdio.h>

#include "common/error.h"
#include "common/utils.h"
#include "lexer/lexer_token.h"
#include "parser/ast_type.h"
#include "parser/parser_parse.h"

#include "lexer.h"
#include "parser.h"

// Use "optimal" size for debug builds. Just so we can test more of our arena implementation.
#ifdef __OPTIMIZE__
#define PARSER_ARENA_CAPACITY 4096
#else
#define PARSER_ARENA_CAPACITY sizeof(ast_node_t) * 2
#endif

parser_t parser_new(lexer_t* lexer) {
    RUNTIME_ASSERT(lexer != NULL, "lexer is null");

    arena_t arena = arena_new(PARSER_ARENA_CAPACITY);
    ast_node_t* root = arena_alloc_zeroed(&arena, sizeof(ast_node_t));
    root->kind = AST_TRANSLATION_UNIT;
    root->data.translation_unit.body = NULL;

    return (parser_t){
        .node_root = root,
        .token_index = 0,
        .lexer = lexer,
        .arena = arena,
    };
}

void parser_cleanup(parser_t* parser) {
    ast_free_tree(parser->node_root);
    arena_free(&parser->arena);
}

void parser_parse(parser_t* parser) {
    RUNTIME_ASSERT(parser->node_root, "root is NULL");
    parser->node_root->data.translation_unit.body = parse_global_scope(parser);
}


