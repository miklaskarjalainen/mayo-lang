#include <stb/stb_ds.h>

#include "../common/error.h"
#include "../compile_error.h"
#include "../lexer/lexer_token.h"
#include "../cli/cli.h"
#include "../parser.h"
#include "parser_error.h"

#include "ast_eval.h"
#include "ast_type.h"
#include "parser_parse.h"

#define MAX_PRECEDENCE 7

static op_t token_to_op_unary(const token_t* tk) {
    switch (tk->kind) {
        case TOK_STAR     : { return UNARY_OP_DEREFERENCE; }
        case TOK_AMPERSAND: { return UNARY_OP_ADDRESS_OF; }

        default: { return OP_INVALID; }
    }
}

static op_t token_to_op_binary(const token_t* tk) {
    switch (tk->kind) {
        case TOK_PLUS : { return BINARY_OP_ADD; }
        case TOK_MINUS: { return BINARY_OP_SUBTRACT; }
        case TOK_STAR : { return BINARY_OP_MULTIPLY; }
        case TOK_SLASH: { return BINARY_OP_DIVIDE; }
        case TOK_MODULO: { return BINARY_OP_MODULO; }

        /* Comparisons */
        case TOK_DOUBLE_EQUAL: { return BINARY_OP_EQUAL; }
        case TOK_BANG_EQUAL  : { return BINARY_OP_NOT_EQUAL; }

        case TOK_DOUBLE_AMPERSAND: { return BINARY_OP_AND; }
        case TOK_DOUBLE_PIPE     : { return BINARY_OP_OR; }

        case TOK_PAREN_OPEN      : { return OP_PAREN_OPEN; }
        case TOK_PAREN_CLOSE     : { return OP_PAREN_CLOSE; }

        case TOK_BRACKET_OPEN    : { return BINARY_OP_ARRAY_INDEX; }
        case TOK_DOT             : { return BINARY_OP_GET_MEMBER; }

        /* Assignment */
        case TOK_EQUALS          : { return BINARY_OP_ASSIGN; }

        default: {
            PANIC("unkown binary operator %s", token_kind_to_str(tk->kind));
            break;
        }
    }
    return OP_INVALID;
}

static uint8_t get_precedence(op_t op) {
    /* higher values have priority over lower ones "1+2*3" -> "1+(2*3)", because '*' has higher precedence than '+' */
    switch (op) {
        /* Unary ops */
        case UNARY_OP_ADDRESS_OF:
        case UNARY_OP_DEREFERENCE:
            return 6;

        /* Binary ops */
        case BINARY_OP_ASSIGN:
            return 0;

        case BINARY_OP_AND:
        case BINARY_OP_OR:
            return 1;
        case BINARY_OP_EQUAL:
        case BINARY_OP_NOT_EQUAL:
            return 2;
        case BINARY_OP_SUBTRACT:
        case BINARY_OP_ADD:
            return 3;
        case BINARY_OP_MULTIPLY:
        case BINARY_OP_DIVIDE:
        case BINARY_OP_MODULO:
            return 4;

        case BINARY_OP_ARRAY_INDEX:
            return 5;

        case BINARY_OP_GET_MEMBER:
            return 6;

        /* parens */
        case OP_PAREN_OPEN:
        case OP_PAREN_CLOSE:
            return MAX_PRECEDENCE;
        
        default: {
            PANIC("unimplemented operator %s", op_to_str(op));
            break;
        }
    }

    return MAX_PRECEDENCE;
}

static ast_node_t* ast_parse_expression(parser_t* parser, uint8_t prec);

static ast_node_t* ast_parse_primary(parser_t* parser) {
    token_t tk = parser_eat(parser);

    switch (tk.kind) {
        case TOK_CONST_VALUE: {
            ast_node_t* ast = ast_arena_new(&parser->arena, AST_CONST_VALUE);
            ast->data.constant = tk.variant;
            return ast;
        }

        case TOK_IDENTIFIER: {
            /* function call */
            const token_t Peeked = parser_peek(parser);
            if (Peeked.kind == TOK_PAREN_OPEN) {
                const char* FnName = variant_get_cstr(&tk.variant);
                return parse_function_call(parser, FnName, true);
            }
            /* struct initializer list */
            if (Peeked.kind == TOK_CURLY_OPEN) {
                const char* TypeName = variant_get_cstr(&tk.variant);
                return parse_struct_initializer_list(parser, TypeName);
            }
            /* regular variable */
            ast_node_t* ast = ast_arena_new(&parser->arena, AST_GET_VARIABLE);
            ast->data.literal = tk.variant.value.literal.chars;
            return ast;
        }

        /* array initializer list */
        case TOK_BRACKET_OPEN: {
            return parse_array_initializer_list(parser);
        }

        case TOK_PAREN_OPEN: {
            ast_node_t* ast = ast_parse_expression(parser, 0);
            token_t other = parser_eat(parser);
            PARSER_ASSERT(other.kind == TOK_PAREN_CLOSE, tk.position, "not closed");
            return ast;
        }

        default: {
            PARSER_ERROR(tk.position, "unexpected token '%s'", token_kind_to_str(tk.kind));
            break;
        }
    }
    return NULL;
}

static ast_node_t* ast_parse_expression(parser_t* parser, uint8_t prec) {
    /* Unary Ops */
    token_t peeked = parser_peek(parser);
    op_t unary_op = token_to_op_unary(&peeked);
    if (unary_op != OP_INVALID && prec == get_precedence(unary_op)) {
        parser_eat_expect(parser, peeked.kind);
        ast_unary_op_t unary = {
            .operation = unary_op,
            .operand   = ast_parse_expression(parser, prec)
        };
        ast_node_t* ast = ast_arena_new(&parser->arena, AST_UNARY_OP);
        ast->data.unary_op = unary;
        return ast;
    }

    if (prec > MAX_PRECEDENCE) {
        return ast_parse_primary(parser);
    }

    ast_node_t* lhs = ast_parse_expression(parser, prec + 1);
    token_t tk = parser_peek(parser);

    /* expr terminators */
    if (
        tk.kind != TOK_NONE &&
        tk.kind != TOK_SEMICOLON && 
        tk.kind != TOK_COMMA &&

        tk.kind != TOK_PAREN_CLOSE &&
        tk.kind != TOK_CURLY_OPEN && 
        tk.kind != TOK_CURLY_CLOSE &&
        tk.kind != TOK_BRACKET_CLOSE 
    )
    {
        op_t op = token_to_op_binary(&tk);
        if (get_precedence(op) == prec) {
            ast_node_t* rhs = NULL;

            parser_eat(parser);
            if (op == BINARY_OP_ARRAY_INDEX) {
                /* parse the expression in brackets as it's own thing. [<expr>] */
                rhs = ast_parse_expression(parser, 0);
                parser_eat_expect(parser, TOK_BRACKET_CLOSE);

                /* multidimensional */
                if (parser_eat_if(parser, TOK_BRACKET_OPEN)) {
                    ast_node_t* new_rhs = ast_parse_expression(parser, 0);
                    parser_eat_expect(parser, TOK_BRACKET_CLOSE);
                    
                    ast_node_t* operator = ast_arena_new(&parser->arena, AST_BINARY_OP);
                    operator->data.binary_op.operation = BINARY_OP_ARRAY_INDEX;
                    operator->data.binary_op.left = lhs;
                    operator->data.binary_op.right = rhs;
                    lhs = operator;
                    rhs = new_rhs;
                }
            }
            else {
                rhs = ast_parse_expression(parser, prec);
            }

            ast_node_t* operator = ast_arena_new(&parser->arena, AST_BINARY_OP);
            operator->data.binary_op.operation = op;
            operator->data.binary_op.left = lhs;
            operator->data.binary_op.right = rhs;
            return operator;
        }
    }

    return lhs;
}

ast_node_t* ast_eval_expr(parser_t* parser) {
    ast_node_t* ast = ast_parse_expression(parser, 0);
    return ast;
}
