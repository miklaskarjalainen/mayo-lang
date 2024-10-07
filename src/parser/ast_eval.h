#ifndef MYLANG_AST_EVAL_H
#define MYLANG_AST_EVAL_H

struct ast_node_t;
struct parser_t;

struct ast_node_t* ast_eval_expr(struct parser_t* parser);

#endif
