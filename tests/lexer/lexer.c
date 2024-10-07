#include <criterion/assert.h>
#include <criterion/criterion.h>
#include <string.h>

#include <stb/stb_ds.h>

#include "../../src/lexer.h"
#include "../../src/lexer/lexer_token.h"
#include "../../src/common/error.h"

#define CODE(code) #code

Test(lexer_tests, string_lexing) {
    #define CHECK_STR_LITERAL(code_literal, idx)                                    \
        do {                                                                        \
            cr_assert((idx+1) <= arrlenu(lexer->tokens));                           \
            cr_assert_eq(lexer->tokens[idx].kind, TOK_VALUE_LITERAL);               \
            cr_assert_str_eq(lexer->tokens[idx].value.literal.chars, code_literal); \
        } while(0)

    char string_literals[] = CODE(
        "Hello World!\n" // basic case
        "\r\n\t\"\'"     // escapes
    );

    lexer_t* lexer = lexer_str(string_literals, NULL);
    RUNTIME_ASSERT(lexer, "Could not create lexer :^(");
    lexer_lex(lexer);

    // Basics
    size_t i = 0;
    CHECK_STR_LITERAL("Hello World!\n", i); i++;
    CHECK_STR_LITERAL("\r\n\t\"\'", i); i++;

    // don't call free on content since it's local variable.
    lexer->content = NULL;
    lexer_delete(lexer);
    #undef CHECK_STR_LITERAL
}

Test(lexer_tests, char_lexing) {
    #define CHECK_CHAR_LITERAL(char_literal, idx)                                   \
        do {                                                                        \
            cr_assert((idx+1) <= arrlenu(lexer->tokens));                           \
            cr_assert_eq(lexer->tokens[idx].kind, TOK_VALUE_CHAR);                  \
            cr_assert_eq(lexer->tokens[idx].value.character, char_literal);         \
        } while(0)

    char char_literals[] = CODE(
        'A''b''C''#'        // basic cases
        '\n''\0''\'''\t'    // escapes
    );

    lexer_t* lexer = lexer_str(char_literals, NULL);
    RUNTIME_ASSERT(lexer, "Could not create lexer :^(");
    lexer_lex(lexer);

    // Basics
    size_t i = 0;
    CHECK_CHAR_LITERAL('A', i); i++;
    CHECK_CHAR_LITERAL('b', i); i++;
    CHECK_CHAR_LITERAL('C', i); i++;
    CHECK_CHAR_LITERAL('#', i); i++;

    // Escapes
    CHECK_CHAR_LITERAL('\n', i); i++;
    CHECK_CHAR_LITERAL('\0', i); i++;
    CHECK_CHAR_LITERAL('\'', i); i++;
    CHECK_CHAR_LITERAL('\t', i); i++;

    lexer->content = NULL;
    lexer_delete(lexer);
    #undef CHECK_CHAR_LITERAL
}

#undef CODE

