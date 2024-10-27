#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <stb/stb_ds.h>

#include "common/arena.h"
#include "common/error.h"
#include "common/string.h"
#include "common/utils.h"
#include "file_position.h"
#include "lexer/lexer_token.h"
#include "compile_error.h"
#include "variant/variant.h"
#include "lexer.h"

#define LEXER_ERROR(pos, ...)                   \
    do {                                        \
        PRINT_ERROR_IN_FILE(pos, __VA_ARGS__);  \
        exit(1);                                \
    } while(0)

void lexer_init(lexer_t* lexer, arena_t* arena, const char* fpath) {
    //
    char* content = read_file_contents(fpath);
    RUNTIME_ASSERT(content != NULL, "Could not read file input :^(");
    lexer_str(lexer, arena, content, fpath);
}

void lexer_str(lexer_t* l, arena_t* arena, char* content, const char* fpath) {
    RUNTIME_ASSERT(content != NULL, "content is NULL");

    // copy filepath
    char* filepath_copied = NULL;
    if (fpath != NULL) {
        const size_t Size = strlen(fpath) + 1;
        filepath_copied = arena_alloc_zeroed(arena, Size);
        RUNTIME_ASSERT(filepath_copied != NULL, "Could not create memory to store the filepath.");
        strcpy(filepath_copied, fpath);
    }
    
    // Initialize members
    l->arena = arena;
    l->tokens = NULL;
    l->word = string_with_capacity(0xFF);
    l->is_commented = COMMENT_NONE;

    l->filepath = filepath_copied;
    l->content = content;
    l->content_pointer = 0;
    l->content_length = strlen(content);

    l->line = 1;
    l->column = 1;
    l->word_begin = lexer_get_position(l);
}

void lexer_cleanup(lexer_t* lexer) {
    DEBUG_ASSERT(lexer != NULL, "Trying to delete NULL");

    arrfree(lexer->tokens);

    // Delete everything else
    string_delete(&lexer->word); 

    // If the content was read from a filepath then it was allocated.
    if (lexer->content && lexer->filepath) {
        free(lexer->content); 
    }
}

char lexer_peek_behind(const lexer_t* lexer) {
    return lexer_peek_by(lexer, -1);
}

char lexer_peek(const lexer_t* lexer) {
    return lexer_peek_by(lexer, 0);
}

char lexer_peek_by(const lexer_t* lexer, int peek_by) {
    const int64_t Idx = (int64_t)lexer->content_pointer + peek_by;
    if (Idx < 0 || Idx >= (int64_t)lexer->content_length) {
        return '\0';
    }
    return lexer->content[Idx];
}

file_position_t lexer_get_position(lexer_t* lexer) {
    return file_pos_new(
        lexer->filepath,
        lexer->line,
        lexer->column,
        -1
    );
}

static void lexer_flush(lexer_t* lexer) {
    if (string_is_empty(&lexer->word)) {
        return;
    }

    // match token
    token_t tk = token_new(
        token_get_keyword(lexer->word.chars)
    );
    tk.position = lexer->word_begin;
    tk.position.length = lexer->word.length;

    // Can we match this to a keyword
    if (tk.kind != TOK_NONE) {
        arrpush(lexer->tokens, tk);
    }
    // Booleans
    else if (!strcmp(lexer->word.chars, "true")) {
        tk.kind = TOK_CONST_BOOLEAN;
        tk.data.boolean = true;
        arrpush(lexer->tokens, tk);
    }
    else if (!strcmp(lexer->word.chars, "false")) {
        tk.kind = TOK_CONST_BOOLEAN;
        tk.data.boolean = false;
        arrpush(lexer->tokens, tk);
    }
    //@TODO: Unsigned integers
    //@TODO: Postfixes like "0u64", "10.0f32" 
    else if (is_integer(lexer->word.chars)) {
        tk.kind = TOK_CONST_INTEGER;
        tk.data.integer = atoi(lexer->word.chars);
        arrpush(lexer->tokens, tk);
    }
    else if (is_floating_point(lexer->word.chars)) {
        tk.kind = TOK_CONST_FLOAT;
        tk.data.f32 = str2f32(lexer->word.chars);
        arrpush(lexer->tokens, tk);
    }
    else {
        // check that an identifier only contains valid characters.
        for (size_t i = 0; i < lexer->word.length; i++) {
            const char c = lexer->word.chars[i];
            if (!issym(c)) {
                // Print error for the character
                file_position_t char_pos = lexer->word_begin;
                char_pos.column += i;
                char_pos.length = 1;
                LEXER_ERROR(char_pos, "Invalid character in identifier.");
            }
        }

        size_t Len = lexer->word.length;
        tk.kind = TOK_IDENTIFIER;
        tk.data.str = arena_alloc_zeroed(lexer->arena, Len + 1);
        strncpy(tk.data.str, lexer->word.chars, Len);
        arrpush(lexer->tokens, tk);
    }

    string_clear(&lexer->word);
}

// get pointer to the currently pointer location of the content_pointer.
const char* lexer_get_ptr(const lexer_t* lexer) {
    if (lexer->content_pointer >= lexer->content_length) {
        return NULL;
    }
    const char* Ptr = lexer->content + lexer->content_pointer;
    return Ptr;
}

// returns true if this 'char' should be skipped because it is in a comment.
// handles eating the char
static bool lexer_handle_comments(lexer_t* lexer) {
    //@FIXME: using comments along symbols might cause funny behaviour.
    //  something like "x +/**/=1;" would never work, because eat_symbol doesn't handle comments at all.

    switch (lexer->is_commented) {
        case COMMENT_NONE: {
            char c = lexer_peek(lexer);

            if (c == '/') {
                char second = lexer_peek_by(lexer, 1);

                if (second == '/') {
                    lexer_flush(lexer);
                    lexer->is_commented = COMMENT_SINGLELINE; 
                    lexer_eat(lexer);
                    lexer_eat(lexer);
                    return true;
                }
                else if (second == '*') { 
                    lexer_flush(lexer);
                    lexer->is_commented = COMMENT_MULTILINE; 
                    lexer_eat(lexer);
                    lexer_eat(lexer);
                    return true;
                }
            }
            return false;
        }

        case COMMENT_SINGLELINE: {
            if (lexer_eat(lexer) == '\n') {
                lexer->is_commented = COMMENT_NONE; 
            }
            return true;
        } 
        
        case COMMENT_MULTILINE: {
            if (lexer_eat(lexer) == '*') {
                // since we're already in a comment eating twice is fine.
                if (lexer_eat(lexer) == '/') { 
                    lexer->is_commented = COMMENT_NONE; 
                }
            }
            return true;
        }
        default:
            DEBUG_ASSERT(false, "should not be possible");
            break; 
    }
    return false;
}

void lexer_lex(lexer_t* lexer) {
    RUNTIME_ASSERT(lexer->tokens == NULL, "tokens is not NULL...");
    while(lexer_peek(lexer) != '\0') {
        if (lexer_handle_comments(lexer)) {
            continue;
        }

        // Symbol like '==' or '+=' was found and causes flush.
        // TODO: symbol length for errors
        const token_kind_t SymbolKind  = lexer_eat_symbol(lexer);
        size_t symbol_start = lexer->column;
        if (SymbolKind != TOK_NONE) {
            // Floats: don't capture DOT after numbers. Push the dot to word.
            // Also having this after eat_symbol, checks cases like "0..9234", which would be invalid and not captured by this.
            if (SymbolKind == TOK_DOT && is_integer(lexer->word.chars)) {
                string_push(&lexer->word, '.');
                continue;
            }            
            lexer_flush(lexer);
            
            token_t tk = token_new(SymbolKind);
            tk.position = lexer_get_position(lexer);
            tk.position.length = strlen(token_kind_to_str(SymbolKind));
            tk.position.column = symbol_start - tk.position.length;
            arrpush(lexer->tokens, tk);
            continue;
        }
        
        char c = lexer_eat(lexer);
        if (c == '"') {
            lexer_flush(lexer);
            token_t tk = lexer_eat_string_literal(lexer);
            arrpush(lexer->tokens, tk);
            continue;
        }
        if (c == '\'') {
            lexer_flush(lexer);
            token_t tk = token_new(TOK_CONST_CHAR);
            tk.position = lexer_get_position(lexer);
            tk.data.c = lexer_eat_char_literal(lexer);
            tk.position.length = 1;  // TODO: symbol length for errors
            arrpush(lexer->tokens, tk);
            continue;
        }
        if (isspace(c)) {
            lexer_flush(lexer);
            continue;
        }

        // No captures, keep constructing our current word.
        string_push(&lexer->word, c);
    }

    RUNTIME_ASSERT(lexer->tokens != NULL, "Lexer was not able to parse any tokens from '%s'. :^(", lexer->filepath);
}
