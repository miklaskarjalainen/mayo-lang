#include <stb/stb_ds.h>

#include "../common/error.h"
#include "../compile_error.h"

#include "../lexer.h"
#include "../parser.h"

void parser_uneat(parser_t* parser) {
    RUNTIME_ASSERT(parser->token_index > 0, "token index underflowed");
    parser->token_index--;
}

token_t parser_peek_behind(const parser_t* parser) {
    return parser_peek_by(parser, -1);
}

token_t parser_peek(const parser_t* parser) {
    return parser_peek_by(parser, 0);
}

token_t parser_peek_by(const parser_t* parser, int peek_by) {
    const int64_t Idx = (int64_t)parser->token_index + peek_by;
    const int64_t TokLen = (int64_t)arrlen(parser->lexer->tokens);
    if (Idx < 0 || Idx >= TokLen) {
        return token_new(TOK_NONE);
    }
    return parser->lexer->tokens[Idx];
}

token_t parser_eat(parser_t* parser) {
    token_t tok = parser_peek(parser);
    parser->token_index += 1;
    return tok;
}

token_t parser_eat_expect(parser_t* parser, token_kind_t expect) {
    token_t tok = parser_eat(parser);

    if (tok.kind != expect) {
        file_position_t pos = tok.position;
        PRINT_ERROR_IN_FILE(pos, "expected '%s' got '%s' instead!", token_kind_to_str(expect), token_kind_to_str(tok.kind));
        exit(-1);
    }

    return tok;
}

bool parser_eat_if(parser_t* parser, token_kind_t expect) {
    token_t tok = parser_peek(parser);
    if (tok.kind == expect) {
        parser_eat(parser);
        return true;
    }
    return false;
}

