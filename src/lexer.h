#ifndef MYLANG_LEXER_H
#define MYLANG_LEXER_H
#include <stddef.h>

#include "lexer/lexer_token.h"

#include "common/string.h"
#include "file_position.h"

struct arena_t;

typedef enum comment_type {
        COMMENT_NONE = 0, 
        COMMENT_SINGLELINE, // singleline comment
        COMMENT_MULTILINE   /* multiline comment */
} comment_type;

typedef struct lexer_t {
    struct arena_t* arena;
    struct token_t* tokens;
    string_t word;
    comment_type is_commented;

    /* Content */
    char* filepath;
    char* content;
    size_t content_pointer;
    size_t content_length;

    /* For Errors */
    size_t line, column;
    file_position_t word_begin; // where the current word started.
} lexer_t;

/* Creation & Deletion */
void lexer_init(lexer_t* lexer, struct arena_t* arena, const char* fpath);
void lexer_str(lexer_t* lexer, struct arena_t* arena, char* content, const char* fpath); // takes ownership of content, fpath is for error messages, pass empty if not needed string!
void lexer_cleanup(lexer_t* lexer);

/* Methods */
file_position_t lexer_get_position(lexer_t* lexer); // get's the current char without popping it
void lexer_lex(lexer_t* lexer);

/* Peek Tokens */
const char* lexer_get_ptr(const lexer_t* lexer); // get pointer to the currently pointer location of the content_pointer.
char lexer_peek(const lexer_t* lexer);
char lexer_peek_by(const lexer_t* lexer, int peek_by);
char lexer_peek_behind(const lexer_t* lexer);

/* Consume tokens */
char lexer_eat(lexer_t* lexer);
char lexer_eat_expect(lexer_t* lexer, char expected);
token_kind_t lexer_eat_symbol(lexer_t* lexer);
char lexer_eat_escaped(lexer_t* lexer);
char lexer_eat_char_literal(lexer_t* lexer);
token_t lexer_eat_string_literal(lexer_t* lexer);

#endif 
