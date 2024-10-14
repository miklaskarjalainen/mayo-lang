#include <string.h>

#include "../common/arena.h"
#include "../common/error.h"
#include "../common/utils.h"
#include "../compile_error.h"
#include "../lexer.h"

#define LEXER_ERROR(pos, ...)                   \
    do {                                        \
        PRINT_ERROR_IN_FILE(pos, __VA_ARGS__);  \
        exit(1);                                \
    } while(0)

char lexer_eat(lexer_t* lexer) {
    if (lexer->content_pointer >= lexer->content_length) {
        return '\0';
    }

    char c = lexer_peek(lexer);
    // Update lines and columns.
    // TODO: confirm behaviour for '\r\n' 
    switch(c) {
    case '\n':
        lexer->line += 1;
        lexer->column = 1;
        break;
    default:
        lexer->column += 1;
        break;
    }
    lexer->content_pointer += 1;

    if (lexer->word.length == 0) {
        lexer->word_begin.line = lexer->line;
        lexer->word_begin.column= lexer->column-1;
    }

    return c;
}

char lexer_eat_expect(lexer_t* lexer, char expected) {
    char c = lexer_eat(lexer);

    if (c != expected) {
        file_position_t pos = lexer_get_position(lexer);
        pos.length = 1;
        LEXER_ERROR(pos, "expected %c got %c instead :^(", expected, c);
    }
    return c;
}

token_kind_t lexer_eat_symbol(lexer_t* lexer) {
    // every possible symbol.
    static struct {
        unsigned int len;
        const char* str;
        token_kind_t kind;
    } symbols[] = {
        #define _TK_SYMBOLS
        #define _DEF(tk, symbol) { .len = sizeof(symbol)-1, .str = symbol, .kind = tk }
        #include "./token_kinds.h"
        #undef _DEF
    };

    // 1. loop through all possible symbols
    // 2. match 
    // 3. return best match (if no matches return TOK_NONE)
    
    // A token which is the best match is the one with most same characters.
    // e.g "1+1" if lexer is at the '+' symbol, then it will match that.
    // "1==1" will match '==' instead of just '='.

    token_kind_t best = TOK_NONE;
    size_t best_len = 0;

    const char* CurrentPoint = lexer_get_ptr(lexer);
    if (CurrentPoint == NULL) {
        return best;
    }

    for (size_t i = 0; i < ARRAY_LEN(symbols); i++) {
        const unsigned int Len = symbols[i].len;

        // we already got a better match
        if (Len <= best_len) { continue; }

        // is match
        if (!strncmp(symbols[i].str, CurrentPoint, Len)) {
            best = symbols[i].kind;
            best_len = Len;
        }
    }

    lexer->content_pointer += best_len;
    lexer->column += best_len;

    return best;
}

char lexer_eat_escaped(lexer_t* lexer) {
    /* Escapes */
    DEBUG_ASSERT(lexer_peek_behind(lexer) == '\\', "escape not started with '\\'");
    file_position_t p = lexer_get_position(lexer);
    char next = lexer_eat(lexer);

    switch(next) {
        case '\\': { return '\\'; }
        case '\'': { return '\''; }
        case '"':  { return '"'; }
        
        case 'b':  { return '\b'; }
        case 't':  { return '\t'; }
        case 'r':  { return '\r'; }
        case 'n':  { return '\n'; }
        case '0':  { return '\0'; }

        default: {
            p.length = 2;
            LEXER_ERROR(p, "invalid string escape");
            break;
        }
    }
    return '\0';
}

char lexer_eat_char_literal(lexer_t* lexer) {
    DEBUG_ASSERT(lexer_peek_behind(lexer) == '\'', "no matching quote was found :^(");

    char c = lexer_eat(lexer);
    if (c == '\\') {
        c =  lexer_eat_escaped(lexer);
    }
    lexer_eat_expect(lexer, '\'');
    return c;
}

token_t lexer_eat_string_literal(lexer_t* lexer) {
    DEBUG_ASSERT(lexer_peek_behind(lexer) == '"', "no matching '\"' was found!");

    /*
        eat chars until either '"' or '\n' or '\0' is hit.
        '\n' and '\0' causes an error.
    */

    file_position_t startpos = lexer_get_position(lexer);

    while (true) {
        char c = lexer_eat(lexer);
        if (c == '"') { break; }

        switch (c) {
            /* Throw error */
            case '\n':
            case '\0': {
                if (string_is_empty(&lexer->word)) {
                    RUNTIME_ASSERT(startpos.column > 0, "literal started at column 1?");
                    startpos.length = 1;
                    startpos.column -= 1;
                    LEXER_ERROR(startpos, "Found singular quote with no meaning.");
                    break;
                }

                startpos.length = lexer->word.length;
                LEXER_ERROR(startpos, "string not closed");
                break;
            }
            
            /* Escapes */
            case '\\': {
                char escaped = lexer_eat_escaped(lexer);
                string_push(&lexer->word, escaped);
                break;
            }
                
            default: {
                string_push(&lexer->word, c);
                break;
            }
        }
    }

    token_t tk = { .kind = TOK_CONST_STRING, .position = lexer_get_position(lexer) };
    {
        const size_t Size = lexer->word.length;
        char* literal = arena_alloc_zeroed(lexer->arena, Size+1);
        strncpy(literal, lexer->word.chars, Size);

        tk.data.str = literal;
        tk.position.length = Size;
        tk.position.column -= Size+1;
    }

    string_clear(&lexer->word);

    return tk;
}
