#include <stb/stb_ds.h>

#include "../common/arena.h"
#include "../common/error.h"

#include "ast_type.h"

typedef void (*fn_on_ast)(ast_node_t* node, const void* additional);

#define AST_BODY_FREE(ast_body)             \
    do {                                    \
        size_t Len = arrlenu(ast_body);     \
        for (size_t i = 0; i < Len; i++) {  \
            _ast_free_node(ast_body[i]);    \
        }                                   \
        arrfree(ast_body);                  \
    } while(0)

//TODO: some way to do this with __VA_ARGS__, probably turn this is into a macro? Using a void* to pass additional args is kinda dirty.
static void _ast_free_node(ast_node_t* parent) {
    if (!parent) {
        return;
    }

    // Print children
    switch (parent->kind) {
        case AST_TRANSLATION_UNIT: {
            AST_BODY_FREE(parent->data.translation_unit.body);
            break;
        }

        case AST_FUNCTION_CALL: {
            AST_BODY_FREE(parent->data.function_call.args);
            break;
        }

        case AST_RETURN: {
            _ast_free_node(parent->data.expr);
            break;
        }

        case AST_IF_STATEMENT: {
            _ast_free_node(parent->data.if_statement.expr);

            AST_BODY_FREE(parent->data.if_statement.body);
            AST_BODY_FREE(parent->data.if_statement.else_body);
            break;
        }

        case AST_BINARY_OP: {
            _ast_free_node(parent->data.binary_op.left);
            _ast_free_node(parent->data.binary_op.right);
            break;
        }

        case AST_FIELD_INITIALIZER: {
            _ast_free_node(parent->data.field_initializer.expr);
            break;
        }

        case AST_VARIABLE_DECLARATION: {
            _ast_free_node(parent->data.variable_declaration.expr);
            break;
        }
        
        case AST_STRUCT_DECLARATION: {
            arrfree(parent->data.struct_declaration.members);
            break;
        }

        case AST_FUNCTION_DECLARATION: {
            arrfree(parent->data.function_declaration.args);
            AST_BODY_FREE(parent->data.function_declaration.body);
            break;
        }

        case AST_WHILE_LOOP: {
            _ast_free_node(parent->data.while_loop.expr);
            AST_BODY_FREE(parent->data.while_loop.body);
            break;
        }

        case AST_FOR_LOOP: {
            AST_BODY_FREE(parent->data.for_loop.body);
            break;
        }

        default: { break; }
    }
}

ast_node_t* ast_arena_new(arena_t* arena, ast_kind_t kind) {
    ast_node_t* ptr = arena_alloc_zeroed(arena, sizeof(ast_node_t));
    ptr->kind = kind;
    return ptr;
}

void ast_free_tree(ast_node_t* node) {
    _ast_free_node(node);
}
