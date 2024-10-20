#include <stb/stb_ds.h>
#include <stdbool.h>
#include <threads.h>

#include "../common/arena.h"
#include "../common/error.h"
#include "../compile_error.h"
#include "../parser.h"

#include "ast_eval.h"
#include "ast_type.h"
#include "parser_error.h"
#include "parser_parse.h"

static ast_node_t* parser_eat_expression(parser_t* parser) {
    return ast_eval_expr(parser);
}

static range_t parser_eat_iterator(parser_t* parser) {
    /*
    @TODO: proper 'iterator' type
     
        Range iterator syntax: <integer-from>..<integer-to>[.step(<uint>)][.rev(<bool>)]
    */

    token_t tok_from = parser_eat_expect(parser, TOK_CONST_INTEGER);
    parser_eat_expect(parser, TOK_DOUBLE_DOT);
    token_t tok_to = parser_eat_expect(parser, TOK_CONST_INTEGER);
    
    return (range_t) {
        .from = tok_from.data.integer,
        .to = tok_to.data.integer,

        /* @TODO: */
        .step = 1,
        .reverse = false
    };
}

/* Returns the new modified type */
static datatype_t* parse_datatype_modifiers(parser_t* parser, datatype_t* inner) {
    /*
    Syntax:
        array: <datatype>[<uint>(,)?...]
        ptr: <datatype>*
        datatype: <ptr> | <array> | <identifier>
    */
    /* ptr */
    if (parser_eat_if(parser, TOK_STAR)) {
        /* @FIXME: parsing type from "let i: i32*= 0" causes funny things, star&eq is tokenized as "TOK_STAR_EQUAL" */
        datatype_t* ptr_type = arena_alloc_zeroed(parser->arena, sizeof(datatype_t));
        ptr_type->kind = DATATYPE_POINTER;
        ptr_type->base = inner;
        return parse_datatype_modifiers(parser, ptr_type);
    }
    /* array */
    else if (parser_eat_if(parser, TOK_BRACKET_OPEN)) {
        const size_t Size = parser_eat_expect(parser, TOK_CONST_INTEGER).data.integer;
        parser_eat_expect(parser, TOK_BRACKET_CLOSE);

        datatype_t* array_type = arena_alloc_zeroed(parser->arena, sizeof(datatype_t));
        array_type->kind = DATATYPE_ARRAY;
        array_type->base = inner;
        array_type->array_size = Size;
        return parse_datatype_modifiers(parser, array_type);
    }

    return inner;    
}

static datatype_t parse_eat_datatype(parser_t* parser) {
/*
    Syntax:
        array: <datatype>[<uint>(,)?...]
        ptr: <datatype>*
        datatype: <ptr> | <array> | <identifier>

    Examples:
        identifier: i32
        ptr: i32*
        dual_ptr: i32**
        array: i32[2]
        array2d: i32[2][2]
        array_of_ptr: i32*[1]
        ptr_to_array: i32[1]*
*/
    /* syntax parsing  */
    const token_t TypenameTok = parser_eat_expect(parser, TOK_IDENTIFIER);
    const char* Typename = TypenameTok.data.str;

    datatype_t* type = arena_alloc_zeroed(parser->arena, sizeof(datatype_t));
    type->typename = Typename;
    type->kind = DATATYPE_PRIMITIVE;

    /* get all modifiers, array, ptr, etc. */
    datatype_t* new_type = parse_datatype_modifiers(parser, type);
    datatype_t dereferenced = *new_type;
    return dereferenced;
}

ast_node_t* parse_import_statement(parser_t* parser) {
    /* 
        Syntax: "#import <string-literal>;"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_IMPORT, "?");

    const token_t Token = parser_eat_expect(parser, TOK_CONST_STRING); 
    parser_eat_expect(parser, TOK_SEMICOLON);

    ast_node_t* out = ast_arena_new(parser->arena, AST_IMPORT);
    out->data.literal = Token.data.str;
    return out;
}

static ast_node_t* parse_return_statement(parser_t* parser) {
    /* 
        Syntax: "return [expression];"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_RETURN, "?");
    file_position_t position = parser_peek_behind(parser).position;

    ast_node_t* expr = parser_eat_expression(parser);
    parser_eat_expect(parser, TOK_SEMICOLON);
    ast_node_t* out = ast_arena_new(parser->arena, AST_RETURN);
    out->data.expr = expr;
    out->position = position;
    return out;
}

static ast_node_t* parse_if_statement(parser_t* parser) {
    /*
        Syntax:
            "if <expr> { <body> } [else [if <expr>] { <body> }]"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_IF, "?");

    ast_node_t* expr = parser_eat_expression(parser);
    ast_node_t** body = parse_body(parser);
    ast_node_t** else_body = NULL;
    
    if (parser_eat_if(parser, TOK_KEYWORD_ELSE)) {
        if (parser_eat_if(parser, TOK_KEYWORD_IF)) {
            arrpush(else_body, parse_if_statement(parser));
        }
        else {
            else_body = parse_body(parser);
        }
    }

    ast_node_t* out = ast_arena_new(parser->arena, AST_IF_STATEMENT);
    out->data.if_statement.expr = expr;
    out->data.if_statement.body = body;
    out->data.if_statement.else_body = else_body;
    return out;
}

ast_node_t* parse_cast_statement(parser_t* parser) {
    /*
        Syntax:
            "cast<[type]>([expression])""
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_IDENTIFIER, "?");
    DEBUG_ASSERT(strcmp(parser_peek_behind(parser).data.str, "cast") == 0, "?");
    const file_position_t Pos = parser_peek_behind(parser).position; 

    parser_eat_expect(parser, TOK_LESS_THAN);
    datatype_t type = parse_eat_datatype(parser);
    parser_eat_expect(parser, TOK_GREATER_THAN);
    parser_eat_expect(parser, TOK_PAREN_OPEN);
    ast_node_t* expr = parser_eat_expression(parser);
    parser_eat_expect(parser, TOK_PAREN_CLOSE);

    ast_node_t* out = ast_arena_new(parser->arena, AST_CAST_STATEMENT);
    out->data.cast_statement = (ast_cast_statement_t){
        .expr = expr,
        .target_type = type
    };
    out->position = Pos;
    return out;
}

ast_node_t* parse_variable_declaration(parser_t* parser) {
    /* 
        Syntax: "let <id>: <type> = <expr>;" 
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_LET, "?");

    const token_t IdentifierTok = parser_eat_expect(parser, TOK_IDENTIFIER); 
    const char* VariableName = IdentifierTok.data.str;
    parser_eat_expect(parser, TOK_COLON);
    const datatype_t DataType = parse_eat_datatype(parser);
    parser_eat_expect(parser, TOK_EQUALS);
    ast_node_t* expr = parser_eat_expression(parser);
    const token_t SemiColon = parser_eat_expect(parser, TOK_SEMICOLON);

    PARSER_ASSERT(expr != NULL, SemiColon.position, "expected expression before ';'");

    ast_node_t* out = ast_arena_new(parser->arena, AST_VARIABLE_DECLARATION);
    out->data.variable_declaration.name = VariableName;
    out->data.variable_declaration.type = DataType;
    out->data.variable_declaration.expr = expr;
    out->position = IdentifierTok.position;
    return out;
}

static ast_node_t* parse_function_parameters(parser_t* parser) {
    ast_node_t* args = NULL;
    while (!parser_eat_if(parser, TOK_PAREN_CLOSE)) {
        /*
            Argument syntax: (disallows trailing commas)
                "<identifier>: <type> [,]"   

                can end in "..." signaling for variadic function.     
        */ 
        if (parser_eat_if(parser, TOK_TRIPLE_DOT)) {
            ast_node_t arg = { 
                .kind = AST_VARIABLE_DECLARATION,
                .position = parser_peek_behind(parser).position
            };
            arg.data.variable_declaration = (ast_variable_declaration_t) { 
                .name = "",
                .type = (datatype_t){.kind = DATATYPE_VARIADIC},
                .expr = NULL
            };
            arrpush(args, arg);
            if (!parser_eat_if(parser, TOK_PAREN_CLOSE)) {
                PARSER_ERROR(parser_peek(parser).position, "There can not be any arguments after the '...'!");
            }
            break;
        }

        const token_t IdentifierTok = parser_eat_expect(parser, TOK_IDENTIFIER);
        const char* Identifier = IdentifierTok.data.str;
        parser_eat_expect(parser, TOK_COLON);
        datatype_t type = parse_eat_datatype(parser);

        /* push arg */
        ast_node_t arg = { 
            .kind = AST_VARIABLE_DECLARATION,
            .position = IdentifierTok.position
        };
        arg.data.variable_declaration = (ast_variable_declaration_t) { 
            .name = Identifier,
            .type = type,
            .expr = NULL
        };

        arrpush(args, arg);

        /* continue? */
        token_t tok = parser_eat(parser);
        if (tok.kind == TOK_PAREN_CLOSE) {
            break;
        }
        else if (tok.kind == TOK_COMMA) {
            PARSER_ASSERT(parser_peek(parser).kind != TOK_PAREN_CLOSE, tok.position, "trailing commas not allowed in function declarations!");
            continue;
        }
        else if (tok.kind == TOK_NONE) {
            PARSER_ERROR(tok.position, "expected a ')' to close a function declaration");
            break;
        }
        else {
            PARSER_ERROR(tok.position, "unexpected token %s", token_kind_to_str(tok.kind));
        }
    }
    return args;
}

static ast_node_t* parse_extern_function_declaration(parser_t* parser) {
    /* 
        Syntax:
            "extern fn <identifier>(<arg1-idf>: <arg1-type>, ...) -> <return-type>;"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_EXTERN, "?");
    parser_eat_expect(parser, TOK_KEYWORD_FN);

    // Definition
    const token_t FunctionName = parser_eat_expect(parser, TOK_IDENTIFIER);
    parser_eat_expect(parser, TOK_PAREN_OPEN);
    ast_node_t* args = parse_function_parameters(parser);
    parser_eat_expect(parser, TOK_ARROW);
    const datatype_t ReturnType = parse_eat_datatype(parser);
    parser_eat_expect(parser, TOK_SEMICOLON);

    // Construction
    ast_node_t* ast = ast_arena_new(parser->arena, AST_FUNCTION_DECLARATION);
    ast->position = FunctionName.position;
    ast->data.function_declaration = (ast_function_declaration_t) {
        .name = FunctionName.data.str,
        .args = args,
        .return_type = ReturnType,
        .body = NULL,
        .external = true
    };
    return ast;
}

static ast_node_t* parse_function_declaration(parser_t* parser) {
    /* 
        Syntax:
            "fn <identifier>(<arg1-idf>: <arg1-type>, ...) -> <return-type> { <body> }"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_FN, "?");


    // Definition
    const token_t FunctionName = parser_eat_expect(parser, TOK_IDENTIFIER);
    parser_eat_expect(parser, TOK_PAREN_OPEN);
    ast_node_t* args = parse_function_parameters(parser);
    parser_eat_expect(parser, TOK_ARROW);
    const datatype_t ReturnType = parse_eat_datatype(parser);
    
    // Body
    ast_node_t** body = parse_body(parser);

    // Construction
    ast_node_t* ast = ast_arena_new(parser->arena, AST_FUNCTION_DECLARATION);
    ast->position = FunctionName.position;
    ast->data.function_declaration = (ast_function_declaration_t) {
        .name = FunctionName.data.str,
        .args = args,
        .return_type = ReturnType,
        .body = body,
        .external = false
    };
    return ast;
}

ast_node_t* parse_array_initializer_list(parser_t* parser) {
    /*
        Syntax: (trailing comma allowed)
            [<expr>(,)...]
    */
    
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_BRACKET_OPEN, "?");

    ast_node_t** exprs = NULL;
    while (!parser_eat_if(parser, TOK_BRACKET_CLOSE)) {
        /* Parsing */
        ast_node_t* expression = parser_eat_expression(parser);
        arrpush(exprs, expression);

        /* Continue? */
        token_t tok = parser_eat(parser);
        if (tok.kind == TOK_BRACKET_CLOSE) {
            break;
        }
        else if (tok.kind == TOK_COMMA) {
            continue;
        }
        else if (tok.kind == TOK_NONE) {
            PARSER_ERROR(tok.position, "expected a ']' to close a array initializer");
            break;
        }
        else {
            PARSER_ERROR(tok.position, "unexpected token %s", token_kind_to_str(tok.kind));
            break;
        }
    }  

    ast_array_initializer_list_t init_list = {
        .exprs = exprs
    };
    ast_node_t* ast = ast_arena_new(parser->arena, AST_ARRAY_INITIALIZER_LIST);
    ast->data.array_initializer_list = init_list;
    return ast;
}

ast_node_t* parse_struct_initializer_list(parser_t* parser, const char* type_identifier) {
    /* 
        Syntax: (trailing comma allowed)
            field: <idf: field>: <expr>
            init_list: <idf: typename> { [<field>][,]... }
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_IDENTIFIER, "?");
    const file_position_t Pos = parser_peek_behind(parser).position;

    ast_field_initializer_t* fields = NULL;
    parser_eat_expect(parser, TOK_CURLY_OPEN);
    while (!parser_eat_if(parser, TOK_CURLY_CLOSE)) {
        /* Parsing */
        const token_t IdentifierTok = parser_eat_expect(parser, TOK_IDENTIFIER);
        const char* FieldIdentifier = IdentifierTok.data.str;
        parser_eat_expect(parser, TOK_COLON);
        ast_node_t* expression = parser_eat_expression(parser);

        /* Push fields */
        ast_field_initializer_t field_assignment = { 
            .name = FieldIdentifier,
            .expr = expression
        };
        arrpush(fields, field_assignment);

        /* Continue? */
        token_t tok = parser_eat(parser);
        if (tok.kind == TOK_CURLY_CLOSE) {
            break;
        }
        else if (tok.kind == TOK_COMMA) {
            continue;
        }
        else if (tok.kind == TOK_NONE) {
            PARSER_ERROR(tok.position, "expected a ')' to close a function declaration");
            break;
        }
        else {
            PARSER_ERROR(tok.position, "unexpected token %s", token_kind_to_str(tok.kind));
        }
    }  

    /* Build ast */
    ast_struct_initializer_list_t init_list = {
        .name = type_identifier,
        .fields = fields
    };
    ast_node_t* ast = ast_arena_new(parser->arena, AST_STRUCT_INITIALIZER_LIST);
    ast->data.struct_initializer_list = init_list;
    ast->position = Pos;
    return ast;
} 

ast_node_t* parse_function_call(parser_t* parser, const char* identifier, bool expr) {
    /* 
        Syntax:
            expression: "<identifier>(<expr>, <expr>, ...)"
            statement : "<identifier>(<expr>, <expr>, ...);"
    */

    const token_t IdentifierTok = parser_peek_behind(parser);
    DEBUG_ASSERT(IdentifierTok.kind == TOK_IDENTIFIER, "?");
    parser_eat_expect(parser, TOK_PAREN_OPEN);
    
    ast_node_t** args = NULL;

    /* Args */
    while (!parser_eat_if(parser, TOK_PAREN_CLOSE)) {
        ast_node_t* expr = parser_eat_expression(parser);
        RUNTIME_ASSERT(expr != NULL, "expected expression");
        arrpush(args, expr);

        token_t tok = parser_eat(parser);
        if (tok.kind == TOK_PAREN_CLOSE) {
            break;
        }
        else if (tok.kind == TOK_COMMA) {
            PARSER_ASSERT(parser_peek(parser).kind != TOK_PAREN_CLOSE, tok.position, "trailing commas not allowed in function calls!");
            continue;
        }
        else if (tok.kind == TOK_NONE) {
            PARSER_ERROR(tok.position, "expected a ')' to close a function call");
            break;
        }
    }

    /* If not part of an expression, expecting a semicolon as well.*/
    if (!expr) {
        parser_eat_expect(parser, TOK_SEMICOLON);
    }

    ast_function_call_t func_call = { 0 };
    func_call.name = identifier;
    func_call.args = args;

    ast_node_t* ast = ast_arena_new(parser->arena, AST_FUNCTION_CALL);
    ast->position = IdentifierTok.position;
    ast->data.function_call = func_call;
    return ast;
}

static ast_node_t* parse_while_loop(parser_t* parser) {
    /* 
        Syntax:
            "while <expr> { <body }"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_WHILE, "?");

    ast_node_t* expr = parser_eat_expression(parser);
    ast_node_t** body = parse_body(parser);

    ast_while_loop_t ast_while_loop = {
        .expr = expr,
        .body = body
    };

    ast_node_t* out = ast_arena_new(parser->arena, AST_WHILE_LOOP);
    out->data.while_loop = ast_while_loop;
    return out;
}

static ast_node_t* parse_for_loop(parser_t* parser) {
    /* 
    TODO: proper 'iterator' type for for loops.
        Syntax:
            "for <identifier> in <iterator> { <body }"
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_FOR, "?");

    
    const token_t IdentifierTok = parser_eat_expect(parser, TOK_IDENTIFIER);
    parser_eat_expect(parser, TOK_KEYWORD_IN);
    range_t range = parser_eat_iterator(parser);
    ast_node_t** body = parse_body(parser);

    /* make the iterator be ast */
    ast_for_loop_t ast_for_loop = {
        .identifier = IdentifierTok.data.str,
        .iter = range,
        .body = body
    };

    ast_node_t* out = ast_arena_new(parser->arena, AST_FOR_LOOP);
    out->data.for_loop = ast_for_loop;
    return out;
}

static ast_node_t* parse_struct_declaration(parser_t* parser) {
    /* 
        Syntax:
            "struct <identifier> { [identifier: type](,)... } "
    */
    DEBUG_ASSERT(parser_peek_behind(parser).kind == TOK_KEYWORD_STRUCT, "?");
    
    const token_t StructIdentifierTok = parser_eat_expect(parser, TOK_IDENTIFIER);
    parser_eat_expect(parser, TOK_CURLY_OPEN);

    ast_variable_declaration_t* members = NULL;
    while (!parser_eat_if(parser, TOK_CURLY_CLOSE)) {
        /*
            Argument syntax: (allows trailing commas)
                "<identifier>: <type> [,]"        
        */ 
        const token_t IdentifierTok = parser_eat_expect(parser, TOK_IDENTIFIER);
        const char* Identifier = IdentifierTok.data.str;
        parser_eat_expect(parser, TOK_COLON);
        datatype_t type = parse_eat_datatype(parser);

        /* push member */
        ast_variable_declaration_t mem = { 
            .name = Identifier,
            .type = type,
            .expr = NULL
        };
        arrpush(members, mem);

        /* continue? */
        token_t tok = parser_eat(parser);
        if (tok.kind == TOK_CURLY_CLOSE) {
            break;
        }
        else if (tok.kind == TOK_COMMA) {
            continue;
        }
        else if (tok.kind == TOK_NONE) {
            PARSER_ERROR(tok.position, "expected a '}' to close a struct declaration");
            break;
        }
        else {
            PARSER_ERROR(tok.position, "unexpected token %s", token_kind_to_str(tok.kind));
        }
    }    

    /* make the iterator be ast */
    ast_struct_declaration_t struct_decl = {
        .name = StructIdentifierTok.data.str,
        .members = members
    };

    ast_node_t* out = ast_arena_new(parser->arena, AST_STRUCT_DECLARATION);
    out->data.struct_declaration = struct_decl;
    return out;
}

ast_node_t** parse_global_scope(parser_t* parser) {
    ast_node_t** global_scope = NULL;

    while (true) {
        token_t tok = parser_eat(parser);
        if (tok.kind == TOK_NONE) { break; }

        switch (tok.kind) {
            case TOK_KEYWORD_IMPORT: {
                ast_node_t* ast = parse_import_statement(parser);
                arrpush(global_scope, ast);
                break;
            }
            case TOK_KEYWORD_LET: {
                ast_node_t* ast = parse_variable_declaration(parser);
                arrpush(global_scope, ast);
                break;
            }
            case TOK_KEYWORD_EXTERN: {
                ast_node_t* ast = parse_extern_function_declaration(parser);
                arrpush(global_scope, ast);
                break;
            }
            case TOK_KEYWORD_FN: {
                ast_node_t* ast = parse_function_declaration(parser);
                arrpush(global_scope, ast);
                break;
            }
            case TOK_KEYWORD_STRUCT: {
                ast_node_t* ast = parse_struct_declaration(parser);
                arrpush(global_scope, ast);
                break;
            }

            /* Global scope errors */
            case TOK_IDENTIFIER:
            case TOK_KEYWORD_WHILE:
            case TOK_KEYWORD_IF: {
                PARSER_ERROR(tok.position, "'%s' not allowed in global scope!", token_kind_to_str(tok.kind));
                break;
            }

            default: {
                PARSER_ERROR(tok.position, "Unhandled token '%s'", token_kind_to_str(tok.kind));
                break;
            }
        }
    }

    return global_scope;
}

ast_node_t** parse_body(parser_t* parser) {
    parser_eat_expect(parser, TOK_CURLY_OPEN);

    ast_node_t** body = NULL;

    for (;;) {
        token_t tok = parser_eat(parser);
        
        if (tok.kind == TOK_CURLY_CLOSE) { break; }
        
        switch(tok.kind) {
            case TOK_KEYWORD_IF: {
                ast_node_t* ast = parse_if_statement(parser);
                arrpush(body, ast);
                break;
            }

            case TOK_KEYWORD_LET: {
                ast_node_t* ast = parse_variable_declaration(parser);
                arrpush(body, ast);
                break;
            }
            case TOK_KEYWORD_RETURN: {
                ast_node_t* ast = parse_return_statement(parser);
                arrpush(body, ast);
                break;
            }
            case TOK_IDENTIFIER: {
                /* 
                    Expressions, which start with an identifier.
                */
                parser_uneat(parser); // restore identifier, so it can be parsed as an expression.
                ast_node_t* ast = parser_eat_expression(parser);
                parser_eat_expect(parser, TOK_SEMICOLON);
                arrpush(body, ast);
                break;
            }
            case TOK_KEYWORD_FOR: {                
                ast_node_t* ast = parse_for_loop(parser);
                arrpush(body, ast);
                break;
            }
            case TOK_KEYWORD_WHILE: {                
                ast_node_t* ast = parse_while_loop(parser);
                arrpush(body, ast);
                break;
            }
            case TOK_KEYWORD_CONTINUE: {
                parser_eat_expect(parser, TOK_SEMICOLON);
                
                ast_node_t* ast = ast_arena_new(parser->arena,AST_CONTINUE);
                arrpush(body, ast);
                break;
            }
            case TOK_KEYWORD_BREAK: {
                parser_eat_expect(parser, TOK_SEMICOLON);

                ast_node_t* ast = ast_arena_new(parser->arena, AST_BREAK);
                arrpush(body, ast);
                break;
            }

            /* Errors */
            case TOK_KEYWORD_FN:
            case TOK_KEYWORD_IMPORT: {
                PARSER_ERROR(tok.position, "'%s' are only allowed in global scope!", token_kind_to_str(tok.kind));
                break;
            }
            case TOK_NONE: {
                PARSER_ERROR(tok.position, "body not closed, add '}'");
                break;
            }
            default: {
                PARSER_ERROR(tok.position, "Unhandled token '%s'", token_kind_to_str(tok.kind));
                break;
            }
        }

    }
    return body;
}
