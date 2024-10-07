#include <criterion/criterion.h>

#include "../../src/lexer/lexer_token.h"

/*
Test(lexer_token_tests, token_match_str) {
    cr_expect_eq(match_token("->").kind, TOK_ARROW);
    cr_expect_eq(match_token("#import").kind, TOK_KEYWORD_IMPORT);
}
*/

Test(lexer_token_tests, kind_to_str) {
    cr_expect_str_eq(token_kind_to_str(TOK_KEYWORD_FN), "fn");
    cr_expect_str_eq(token_kind_to_str(TOK_KEYWORD_IMPORT), "#import");

    cr_expect_str_eq(token_kind_to_str(TOK_DOUBLE_DOT), "..");
    cr_expect_str_eq(token_kind_to_str(TOK_ARROW), "->");

    cr_expect_str_eq(token_kind_to_str(TOK_PAREN_OPEN), "(");
    cr_expect_str_eq(token_kind_to_str(TOK_PAREN_CLOSE), ")");
    cr_expect_str_eq(token_kind_to_str(TOK_CURLY_OPEN), "{");
    cr_expect_str_eq(token_kind_to_str(TOK_CURLY_CLOSE), "}");
    cr_expect_str_eq(token_kind_to_str(TOK_BRACKET_OPEN), "[");
    cr_expect_str_eq(token_kind_to_str(TOK_BRACKET_CLOSE), "]");
}

