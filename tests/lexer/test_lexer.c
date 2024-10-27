#include <criterion/assert.h>
#include <criterion/criterion.h>
#include <string.h>

#include <stb/stb_ds.h>

#include "lexer.h"
#include "lexer/lexer_token.h"
#include "common/arena.h"

#define INITIALIZE_LEXER(code)              \
    arena_t arena; lexer_t lexer;           \
    arena_init(&arena, 0xFF);               \
    lexer_str(&lexer, &arena, code, NULL);  \
    lexer_lex(&lexer)

#define CLEANUP_LEXER()     \
    lexer_cleanup(&lexer);  \
    arena_free(&arena)

#define CODE(code) #code

Test(lexer_tests, string_lexing) {
    char string_literals[] = CODE(
        "Hello World!\n" // basic case
        "\r\n\t\"\'"     // escapes
    );

    INITIALIZE_LEXER(string_literals);

    // Tests
    {
        cr_assert(arrlenu(lexer.tokens) == 2, "wrong amount of tokens parsed! got %zu.", arrlenu(lexer.tokens));
        cr_assert_str_eq(lexer.tokens[0].data.str, "Hello World!\n", "unexpected value");
        cr_assert_str_eq(lexer.tokens[1].data.str, "\r\n\t\"\'", "unexpected value");
    }
    
    CLEANUP_LEXER();
}

Test(lexer_tests, char_lexing) {
    char char_literals[] = CODE(
        'A''b''C''#'        // basic cases
        '\n''\0''\'''\t'    // escapes
    );

    INITIALIZE_LEXER(char_literals);

    // Tests
    {
        // Basics
        cr_assert(arrlenu(lexer.tokens) == 8, "wrong amount of tokens parsed! got %zu.", arrlenu(lexer.tokens));
        cr_expect(lexer.tokens[0].data.c == 'A', "unexpected value");
        cr_expect(lexer.tokens[1].data.c == 'b', "unexpected value");
        cr_expect(lexer.tokens[2].data.c == 'C', "unexpected value");
        cr_expect(lexer.tokens[3].data.c == '#',  "unexpected value");
    
    
        // Escapes
        cr_expect(lexer.tokens[4].data.c == '\n', "unexpected value");
        cr_expect(lexer.tokens[5].data.c == '\0', "unexpected value");
        cr_expect(lexer.tokens[6].data.c == '\'', "unexpected value");
        cr_expect(lexer.tokens[7].data.c == '\t', "unexpected value");
    }

    CLEANUP_LEXER();
}

#undef CODE
