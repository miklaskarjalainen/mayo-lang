#ifndef MAYO_IMPL_GEN_H
#define MAYO_IMPL_GEN_H

#include <stdio.h>
#include "../backend_qbe.h"

struct ast_struct_declaration_t;
struct backend_ctx_t;

temporary_t qbe_generate_string_literal(FILE* f, const struct ast_node_t* ast);
temporary_t qbe_generate_array_initializer(FILE* f, const struct ast_node_t* ast, backend_ctx_t* ctx);
void qbe_generate_struct_members(FILE* f, const struct ast_struct_declaration_t* decl, struct backend_ctx_t* ctx);
temporary_t qbe_generate_function_call(FILE* f, const struct ast_node_t* ast, backend_ctx_t* ctx);
temporary_t qbe_generate_while_loop(FILE* f, const struct ast_node_t* ast, backend_ctx_t* ctx);
temporary_t qbe_generate_if_statement(FILE* f, const struct ast_node_t* ast, backend_ctx_t* ctx);

#endif
