#ifndef MYLANG_LEXER_TOKEN_H
#define MYLANG_LEXER_TOKEN_H

#include <stdint.h>
#include <stdbool.h>

#include "../file_position.h"

typedef enum token_kind_t {
#define _DEF(internal, string) internal
    #include "token_kinds.h"
#undef _DEF
} token_kind_t;

typedef struct token_t {
    token_kind_t kind;
    union {
        bool boolean;
        char c;
        int64_t integer;
        char* str; 
    } data;
    file_position_t position;
} token_t;

const char* token_kind_to_str(token_kind_t kind); // TOK_IMPORT -> "#import"
const char* token_kind_to_str_internal(token_kind_t kind); // TOK_IMPORT -> "TOK_IMPORT"
token_kind_t token_kind_from_str(const char* str);
token_kind_t token_get_keyword(const char* str);

token_t token_new(token_kind_t kind);
void token_clear(token_t* tk); // Clears the token (deletes internal pointers etc), but can be reused.

void token_print_pretty(const token_t* tk);

#endif
