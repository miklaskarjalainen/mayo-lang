/*
    All the tokens are defined here...
    All tokens are defined such:
    #define _DEF(TOK_VALUE, src_string_version) // e.g _DEF(TOK_KEYWORD_IMPORT, "#import")
    So we can use this file to get all tokens(or only some) and map them to where ever we want..

    Define one of these the extract specific tokens. Everything is undefined at the end of this file (so no need for #undef).
        _ALL_TOKENS *default if nothing is defined*
        _TK_KEYWORDS
        _TK_VALUES
        _TK_SYMBOLS
        _TK_MISC

Examples of usage:
The tokens are defined in (lexer_token.h). 
Like this: 
    typedef enum token_kind_t {
    #define _DEF(internal, string) internal
        #include "token_kinds.h"
    #undef _DEF
    } token_kind_t;

Lexer's symbol matching.
Like this: (lexer.c:lexer_eat_symbol)
    static struct {
        unsigned int len;
        const char* str;
        token_kind_t kind;
    } symbols[] = {
        #define _TK_SYMBOLS
        #define _DEF(tk, symbol) { .len = sizeof(symbol)-1, .str = symbol, .kind = tk }
        #include "./lexer/token_kinds.h"
        #undef _DEF
    };
*/

#if !defined(_ALL_TOKENS) && !defined(_TK_KEYWORDS) && !defined(_TK_VALUES) && !defined(_TK_SYMBOLS) && !defined(_TK_MISC)
    #define _ALL_TOKENS
#endif

#if defined(_ALL_TOKENS)
    #define _TK_KEYWORDS
    #define _TK_VALUES
    #define _TK_SYMBOLS
    #define _TK_MISC
#endif

#ifdef _DEF
#if defined(_TK_MISC)
_DEF(TOK_NONE, ""),
_DEF(TOK_IDENTIFIER, "<identifier>"),
#endif

#if defined(_TK_KEYWORDS)
/* Keywords */
_DEF(TOK_KEYWORD_IMPORT, "#import"),
_DEF(TOK_KEYWORD_INCLUDE, "#include"),
_DEF(TOK_KEYWORD_FN, "fn"),
_DEF(TOK_KEYWORD_RETURN, "return"),
_DEF(TOK_KEYWORD_CONTINUE, "continue"),
_DEF(TOK_KEYWORD_BREAK, "break"),
_DEF(TOK_KEYWORD_IF, "if"),
_DEF(TOK_KEYWORD_ELSE, "else"),
_DEF(TOK_KEYWORD_WHILE, "while"),
_DEF(TOK_KEYWORD_FOR, "for"),
_DEF(TOK_KEYWORD_IN, "in"),
_DEF(TOK_KEYWORD_CONST, "const"),
_DEF(TOK_KEYWORD_LET, "let"),
_DEF(TOK_KEYWORD_STRUCT, "struct"),
_DEF(TOK_KEYWORD_EXTERN, "extern"),
#endif

#if defined(_TK_VALUES)
/* value tokens */
_DEF(TOK_CONST_INTEGER, "<const int>"),
_DEF(TOK_CONST_FLOAT, "<const float>"),
_DEF(TOK_CONST_STRING, "<const string>"),
_DEF(TOK_CONST_CHAR, "<const char>"),
_DEF(TOK_CONST_BOOLEAN, "<const boolean>"),
#endif

#if defined(_TK_SYMBOLS)
/* Multi Char Symbols */
_DEF(TOK_TRIPLE_DOT, "..."),
_DEF(TOK_DOUBLE_DOT, ".."),
_DEF(TOK_ARROW, "->"),

/* Operators */
_DEF(TOK_BANG, "!"),
_DEF(TOK_BANG_EQUAL, "!="),

_DEF(TOK_EQUALS, "="),
_DEF(TOK_DOUBLE_EQUAL, "=="),

_DEF(TOK_PLUS, "+"),
_DEF(TOK_PLUS_EQUAL, "+="),

_DEF(TOK_MINUS, "-"),
_DEF(TOK_MINUS_EQUAL, "-="),

_DEF(TOK_STAR, "*"),
_DEF(TOK_STAR_EQUAL, "*="),

_DEF(TOK_SLASH, "/"),
_DEF(TOK_SLASH_EQUAL, "/="),

_DEF(TOK_MODULO, "%"),
_DEF(TOK_MODULO_EQUAL, "%="),

_DEF(TOK_PIPE, "|"), 
_DEF(TOK_PIPE_EQUAL, "|="),
_DEF(TOK_DOUBLE_PIPE, "||"),

_DEF(TOK_AMPERSAND, "&"),
_DEF(TOK_AMPERSAND_EQUAL, "&="),
_DEF(TOK_DOUBLE_AMPERSAND, "&&"),

_DEF(TOK_GREATER_THAN, ">"),
_DEF(TOK_GREATER_THAN_EQUAL, ">="),
_DEF(TOK_DOUBLE_GREATER_THAN, ">>"),
_DEF(TOK_DOUBLE_GREATER_THAN_EQUAL, ">>="),

_DEF(TOK_LESS_THAN, "<"),
_DEF(TOK_LESS_THAN_EQUAL, "<="),
_DEF(TOK_DOUBLE_LESS_THAN, "<<"),
_DEF(TOK_DOUBLE_LESS_THAN_EQUAL, "<<="),

_DEF(TOK_TILDE, "~"),
_DEF(TOK_TILDE_EQUAL, "~="),

_DEF(TOK_CARET, "^"),
_DEF(TOK_CARET_EQUAL, "^="),

/* Single Char Symbols */
_DEF(TOK_PAREN_OPEN, "("),
_DEF(TOK_PAREN_CLOSE, ")"),
_DEF(TOK_CURLY_OPEN, "{"),
_DEF(TOK_CURLY_CLOSE, "}"),
_DEF(TOK_BRACKET_OPEN, "["),
_DEF(TOK_BRACKET_CLOSE, "]"),
_DEF(TOK_COMMA, ","),
_DEF(TOK_SEMICOLON, ";"),
_DEF(TOK_DOT, "."),
_DEF(TOK_COLON, ":"),

#endif

#if defined(_TK_MISC)
_DEF(TOK_COUNT, "TOK_COUNT"),
#endif
#endif

#if defined(_ALL_TOKENS)
    #undef _ALL_TOKENS
#endif

#if defined(_TK_KEYWORDS)
    #undef _TK_KEYWORDS
#endif

#if defined(_TK_VALUES)
    #undef _TK_VALUES
#endif

#if defined(_TK_SYMBOLS)
    #undef _TK_SYMBOLS
#endif

#if defined(_TK_MISC)
    #undef _TK_MISC
#endif

