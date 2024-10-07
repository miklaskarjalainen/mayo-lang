#ifndef MYLANG_PARSER_H
#define MYLANG_PARSER_H

#include "parser/ast_type.h"
#include "lexer/lexer_token.h"

struct lexer_t;

typedef struct parser_t {
    ast_node_t* node_root;
    size_t token_index;
    struct lexer_t* lexer;
} parser_t;

parser_t parser_new(struct lexer_t* lexer);
void parser_cleanup(parser_t* parser);
void parser_parse(parser_t* parser);

/* Peek Tokens */
token_t parser_peek_behind(const parser_t* parser);
token_t parser_peek(const parser_t* parser);
token_t parser_peek_by(const parser_t* parser, int peek_by);

/* Consume Tokens */
void parser_uneat(parser_t* parser); 
token_t parser_eat(parser_t* parser);
token_t parser_eat_expect(parser_t* parser, token_kind_t expect);
bool parser_eat_if(parser_t* parser, token_kind_t expect);

#endif
