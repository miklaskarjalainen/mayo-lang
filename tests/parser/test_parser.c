#include <criterion/criterion.h>
#include <stb/stb_ds.h>

#include "lexer.h"
#include "parser.h"
#include "common/arena.h"

#define CODE(code) #code

#define INITIALIZE_PARSER(code)                     \
    arena_t arena; lexer_t lexer;                   \
    arena_init(&arena, 0xFF);                       \
    lexer_str(&lexer, &arena, code, NULL);          \
    lexer_lex(&lexer);                              \
    parser_t parser = parser_new(&arena, &lexer);   \
    parser_parse(&parser)

#define CLEANUP_PARSER()        \
    parser_cleanup(&parser);    \
    lexer_cleanup(&lexer);      \
    arena_free(&arena)


Test(parser_tests, parser_fn_decl) {
    char* code = CODE(
        fn sum(a: i32, b: i32) -> i32 { return a + b; }
    );

    INITIALIZE_PARSER(code);

    cr_expect(parser.node_root->kind == AST_TRANSLATION_UNIT);
    
    {
        ast_node_t** body = parser.node_root->data.translation_unit.body;
        cr_expect(arrlenu(body) == 1);
        cr_expect(body[0]->kind == AST_FUNCTION_DECLARATION);

        ast_function_declaration_t* decl = &body[0]->data.function_declaration;
        cr_expect_str_eq(decl->name, "sum");
        cr_expect_str_eq(decl->return_type.typename, "i32");
        
        cr_expect(arrlenu(decl->args) == 2);
        cr_expect(decl->args[0].kind == AST_VARIABLE_DECLARATION);
        cr_expect(decl->args[1].kind == AST_VARIABLE_DECLARATION);
        cr_expect_str_eq(decl->args[0].data.variable_declaration.name, "a");
        cr_expect_str_eq(decl->args[1].data.variable_declaration.name, "b");
        cr_expect_str_eq(decl->args[0].data.variable_declaration.type.typename, "i32");
        cr_expect_str_eq(decl->args[1].data.variable_declaration.type.typename, "i32");
    }

    CLEANUP_PARSER();

}
