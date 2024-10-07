#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "../lexer.h"
#include "../common/utils.h"

#include "lexer_token.h"

// e.g TOK_IMPORT -> "TOK_IMPORT"
static const char* TOKEN_INTERNAL_STR[] = {
    #define _DEF(internal, str) [internal] = #internal
    #include "token_kinds.h"
    #undef _DEF
};

// e.g TOK_IMPORT -> "#import"
static const char* TOKEN_STR_MAP[] = {
    #define _DEF(internal, str) [internal] = str
    #include "token_kinds.h"
    #undef _DEF
};

token_kind_t token_kind_from_str(const char* str) {
    for (size_t i = 1; i < ARRAY_LEN(TOKEN_STR_MAP)-1; i++) {
        // match
        if (!strcmp(str, TOKEN_STR_MAP[i])) {
            return (token_kind_t)i;
        }
    }

    return TOK_NONE;
}

const char* token_kind_to_str(token_kind_t kind) {
    const size_t IDX = (size_t)kind;
    assert(IDX < TOK_COUNT);
    return TOKEN_STR_MAP[IDX];
}

const char* token_kind_to_str_internal(token_kind_t kind) {
    const size_t IDX = (size_t)kind;
    assert(IDX < TOK_COUNT);
    return TOKEN_INTERNAL_STR[IDX];
}

token_kind_t token_get_keyword(const char* str) {
    static struct {
        const char* str;
        token_kind_t kind;
    } sKeywords[] = {
        #define _TK_KEYWORDS
        #define _DEF(tk, symbol) { .str = symbol, .kind = tk }
        #include "./token_kinds.h"
        #undef _DEF
    };

    for (size_t i = 0; i < ARRAY_LEN(sKeywords); i++) {
        // match
        if (!strcmp(str, sKeywords[i].str)) {
            return sKeywords[i].kind;
        }
    }

    return TOK_NONE;
}

token_t token_new(token_kind_t kind) {
    return (token_t) {
        .kind = kind,
        .position = file_pos_new(NULL, 0, 0, -1)
    };
}

void token_clear(token_t* tk) {
    variant_cleanup(&tk->variant);
    tk->kind = TOK_NONE;
}

void token_print_pretty(const token_t* tk) {
    printf("[ %s", token_kind_to_str_internal(tk->kind));

    switch (tk->kind) {
        case TOK_CONST_VALUE: {
            printf(", ");
            core_type_print(&tk->variant);
            break;
        }
        case TOK_IDENTIFIER: {
            printf(", " STDOUT_CYAN "%s" STDOUT_RESET, tk->variant.value.literal.chars);
            break;
        }

        default: { break; }
    }
    printf(", line: %zu column: %zu len: %i ]\n", tk->position.line, tk->position.column, tk->position.length);
}
