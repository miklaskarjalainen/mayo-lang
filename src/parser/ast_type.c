#include <stb/stb_ds.h>

#include "../common/error.h"

#include "ast_type.h"

typedef void (*fn_on_ast)(ast_node_t* node, const void* additional);

//TODO: some way to do this with __VA_ARGS__, probably turn this is into a macro? Using a void* to pass additional args is kinda dirty.
static void _ast_call_on_all_children(ast_node_t* parent, fn_on_ast fn, const void* additional) {
    // Print children
    switch (parent->kind) {
        case AST_FUNCTION_CALL: {
            size_t Len = arrlenu(parent->data.function_call.args);
            for (size_t i = 0; i < Len; i++) {
                fn(parent->data.function_call.args[i], additional);
            }
            break;
        }

        case AST_RETURN: {
            fn(parent->data.expr, additional);
            break;
        }

        case AST_IF_STATEMENT: {
            fn(parent->data.if_statement.expr, additional);
            size_t Len = arrlenu(parent->data.if_statement.body);
            for (size_t i = 0; i < Len; i++) {
                fn(parent->data.if_statement.body[i], additional);
            }
            break;
        }

        case AST_BINARY_OP: {
            fn(parent->data.binary_op.left, additional);
            fn(parent->data.binary_op.right, additional);
            break;
        }

        case AST_FIELD_INITIALIZER: {
            fn(parent->data.field_initializer.expr, additional);
            break;
        }

        case AST_VARIABLE_DECLARATION: {
            fn(parent->data.variable_declaration.expr, additional);
            break;
        }

        case AST_FUNCTION_DECLARATION: {
            size_t Len = arrlenu(parent->data.function_declaration.body);
            for (size_t i = 0; i < Len; i++) {
                fn(parent->data.function_declaration.body[i], additional);
            }
            break;
        }

        case AST_WHILE_LOOP: {
            fn(parent->data.while_loop.expr, additional);
            size_t Len = arrlenu(parent->data.while_loop.body);
            for (size_t i = 0; i < Len; i++) {
                fn(parent->data.while_loop.body[i], additional);
            }
            break;
        }

        case AST_FOR_LOOP: {
            size_t Len = arrlenu(parent->data.for_loop.body);
            for (size_t i = 0; i < Len; i++) {
                fn(parent->data.for_loop.body[i], additional);
            }
            break;
        }
        default: { break; }
    }
}

static void _ast_free_internal(ast_node_t* node, const void* _additional) {
    UNUSED(_additional);
    _ast_call_on_all_children(node, _ast_free_internal, NULL);
    
    if (node->kind == AST_FUNCTION_DECLARATION) {
        arrfree(node->data.function_declaration.args);
    }
    else if (node->kind == AST_FUNCTION_CALL) {
        arrfree(node->data.function_call.args);
    }
    else if (node->kind == AST_STRUCT_INITIALIZER_LIST) {
        arrfree(node->data.struct_initializer_list.fields);
    }
    
    free(node);
}

ast_node_t* ast_new(ast_kind_t kind) {
    ast_node_t* ptr = calloc(1, sizeof(ast_node_t));
    ptr->kind = kind;
    return ptr;
}

void ast_free_tree(ast_node_t* node) {
    _ast_free_internal(node, NULL);
}
