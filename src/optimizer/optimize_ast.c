#include <stb/stb_ds.h>
#include "optimize.h"

#include "../cli/cli.h"
#include "../parser/ast_type.h"

#define DO_ON_CHILDREN(body, code)                      \
    do {                                                \
        size_t _count = arrlenu(body);                  \
        for (size_t _it = 0; _it < _count; _it++ ) {    \
            code;                                       \
        }                                               \
    } while(0)

static void _ast_constant_folding(struct ast_node_t* ast) {
    switch (ast->kind) {
        case AST_TRANSLATION_UNIT: {
            DO_ON_CHILDREN(
                ast->data.translation_unit.body, 
                _ast_constant_folding(ast->data.translation_unit.body[_it])
            );
            break;
        }
        case AST_FUNCTION_DECLARATION: {
            DO_ON_CHILDREN(
                ast->data.function_declaration.body, 
                _ast_constant_folding(ast->data.function_declaration.body[_it])
            );
            break;
        }
        case AST_IF_STATEMENT: {
            DO_ON_CHILDREN(
                ast->data.if_statement.body, 
                _ast_constant_folding(ast->data.if_statement.body[_it])
            );
            DO_ON_CHILDREN(
                ast->data.if_statement.else_body, 
                _ast_constant_folding(ast->data.if_statement.else_body[_it])
            );
            break;
        }
        case AST_WHILE_LOOP: {
            _ast_constant_folding(ast->data.expr);
            DO_ON_CHILDREN(
                ast->data.while_loop.body, 
                _ast_constant_folding(ast->data.while_loop.body[_it])
            );
            break;
        }
        case AST_VARIABLE_DECLARATION: {
            _ast_constant_folding(ast->data.variable_declaration.expr);
            break;
        }
        case AST_RETURN: {
            _ast_constant_folding(ast->data.expr);
            break;
        }

        case AST_BINARY_OP: {
            _ast_constant_folding(ast->data.binary_op.left);
            _ast_constant_folding(ast->data.binary_op.right);
            
            const ast_node_t* Lhs = ast->data.binary_op.left;
            const ast_node_t* Rhs = ast->data.binary_op.right;

            ast_kind_t lkind = ast->data.binary_op.left->kind;
            ast_kind_t rkind = ast->data.binary_op.right->kind;
            
            if (lkind == AST_BOOL_LITERAL && rkind == AST_BOOL_LITERAL) {
                switch(ast->data.binary_op.operation) {
                    case BINARY_OP_EQUAL: {
                        ast->kind = AST_BOOL_LITERAL;
                        ast->data.boolean = Lhs->data.boolean == Rhs->data.boolean;
                        break;
                    }
                    case BINARY_OP_NOT_EQUAL: {
                        ast->kind = AST_BOOL_LITERAL;
                        ast->data.boolean = Lhs->data.boolean != Rhs->data.boolean;
                        break;
                    }

                    default: { break; }
                }
            }

            if (lkind == AST_INTEGER_LITERAL && rkind == AST_INTEGER_LITERAL) {
                switch(ast->data.binary_op.operation) {
                    case BINARY_OP_ADD: { 
                        ast->kind = AST_INTEGER_LITERAL;
                        ast->data.integer = Lhs->data.integer + Rhs->data.integer;
                        break;
                    }
                    case BINARY_OP_SUBTRACT: { 
                        ast->kind = AST_INTEGER_LITERAL;
                        ast->data.integer = Lhs->data.integer - Rhs->data.integer;
                        break;
                    }
                    case BINARY_OP_MODULO: { 
                        ast->kind = AST_INTEGER_LITERAL;
                        ast->data.integer = Lhs->data.integer % Rhs->data.integer;
                        break;
                    }

                    case BINARY_OP_EQUAL: {
                        ast->kind = AST_BOOL_LITERAL;
                        ast->data.boolean = Lhs->data.integer == Rhs->data.integer;
                        break;
                    }
                    case BINARY_OP_NOT_EQUAL: {
                        ast->kind = AST_BOOL_LITERAL;
                        ast->data.boolean = Lhs->data.integer != Rhs->data.integer;
                        break;
                    }
                    default: { break; }
                }
            }

            break;
        }



        default : {
            // Optimization either not implemented or available for this node.
        }
    }


}

void perform_ast_optimizations(struct ast_node_t* ast) {
    if (g_Params.opt_ast_constant_folding) {
        _ast_constant_folding(ast);
    }
}
