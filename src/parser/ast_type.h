#ifndef MYLANG_AST_TYPE_H
#define MYLANG_AST_TYPE_H

#include "../common/range.h"
#include "../common/string.h"
#include "../variant/variant.h"
#include "../file_position.h"

typedef enum ast_kind_t {
#define _DEF(internal) internal
    #include "ast_kinds.h"
#undef _DEF
} ast_kind_t;

typedef enum op_t {
#define _DEF(internal) internal
    #include "ops.h"
#undef _DEF
} op_t;

const char* op_to_str(op_t op);
const char* ast_kind_to_str(ast_kind_t kind);

typedef struct ast_binary_op_t {
    op_t operation;
    struct ast_node_t* left;
    struct ast_node_t* right;
} ast_binary_op_t;

typedef struct ast_unary_op_t {
    op_t operation;
    struct ast_node_t* operand;
} ast_unary_op_t;

typedef struct ast_if_statement_t {
    struct ast_node_t* expr;
    struct ast_node_t** body;
    struct ast_node_t** else_body;
} ast_if_statement_t;

typedef struct ast_variable_declaration_t {
    const char* name;
    datatype_t type;
    struct ast_node_t* expr;
} ast_variable_declaration_t;

typedef struct ast_field_initializer_t {
    const char* name;
    struct ast_node_t* expr;
} ast_field_initializer_t;

typedef struct ast_function_declaration_t {
    const char* name;
    struct ast_node_t* args;
    datatype_t return_type;
    struct ast_node_t** body;
} ast_function_declaration_t;

typedef struct ast_struct_declaration_t {
    const char* name;
    ast_variable_declaration_t* members;
} ast_struct_declaration_t;

typedef struct ast_struct_initializer_list_t  {
    const char* name;
    ast_field_initializer_t* fields;
} ast_struct_initializer_list_t;

typedef struct ast_array_initializer_list_t  {
    struct ast_node_t** exprs;
} ast_array_initializer_list_t;

typedef struct ast_function_call_t {
    const char* name;
    struct ast_node_t** args;
} ast_function_call_t;

typedef struct ast_while_loop_t {
    struct ast_node_t* expr;
    struct ast_node_t** body;
} ast_while_loop_t;

typedef struct ast_for_loop_t {
    const char* identifier;
    struct range_t iter;
    struct ast_node_t** body;
} ast_for_loop_t;

typedef struct ast_translation_unit_t {
    struct ast_node_t** body;
} ast_translation_unit_t;

typedef struct ast_node_t {
    ast_kind_t kind;
    union {
        /* Generic */
        variant_t constant;
        const char* literal;
        struct ast_node_t* expr;
        range_t range; // @TODO: proper 'iterator' type.
        int64_t integer;

        /* Ast types */
        ast_unary_op_t unary_op;
        ast_binary_op_t binary_op;
        ast_if_statement_t if_statement;
        ast_variable_declaration_t variable_declaration;
        ast_field_initializer_t field_initializer;
        ast_function_declaration_t function_declaration;
        ast_struct_declaration_t struct_declaration;
        ast_function_call_t function_call;
        ast_translation_unit_t translation_unit;
        ast_while_loop_t while_loop;
        ast_for_loop_t for_loop;
        ast_struct_initializer_list_t struct_initializer_list;
        ast_array_initializer_list_t array_initializer_list;
    } data;

    file_position_t position;
} ast_node_t;

struct arena_t;

ast_node_t* ast_arena_new(struct arena_t* arena, ast_kind_t kind);
void ast_free_tree(ast_node_t* node); // frees all it's children as well

#endif
