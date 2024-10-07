#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "../common/error.h"
#include "../common/utils.h"

#include "core_type.h"
#include "variant.h"

// Mapping
static const char* CORETYPES[] = {
    [CORETYPE_INVALID] = "invalid_type",

    /* Simple */
    [CORETYPE_VOID] = "void",
    [CORETYPE_BOOL] = "bool",
    [CORETYPE_CHAR] = "char",

    /* Integers */
    [CORETYPE_I32] = "i32",
    [CORETYPE_I64] = "i64",
    [CORETYPE_U32] = "u32",
    [CORETYPE_U64] = "u64",
    [CORETYPE_USIZE] = "usize",

    /* Floats */
    [CORETYPE_F32] = "f32",
    [CORETYPE_F64] = "f64",

    [CORETYPE_STR] = "String",

    [CORETYPE_COUNT] = "CORETYPE_COUNT",
};

unsigned int core_type_size(core_type_t core_type) {
    static const unsigned int sSizes[] = {
        [CORETYPE_INVALID] = 0,

        /* Simple */
        [CORETYPE_VOID] = 0,
        [CORETYPE_BOOL] = 1,
        [CORETYPE_CHAR] = 1,

        /* Integers */
        [CORETYPE_I32] = 4,
        [CORETYPE_I64] = 8,
        [CORETYPE_U32] = 4,
        [CORETYPE_U64] = 8,
        [CORETYPE_USIZE] = 8,

        /* Floats */
        [CORETYPE_F32] = 4,
        [CORETYPE_F64] = 8,

        [CORETYPE_STR] = 0,

        [CORETYPE_COUNT] = 0,
    };

    const size_t IDX = (size_t)core_type;
    assert(IDX < CORETYPE_COUNT);
    return sSizes[IDX];
}

const char* core_type_to_str(core_type_t core_type) {
    const size_t IDX = (size_t)core_type;
    assert(IDX < CORETYPE_COUNT);
    return CORETYPES[IDX];
}

core_type_t core_type_from_str(const char* str) {
    for (size_t i = 0; i < ARRAY_LEN(CORETYPES); i++) {
        // match
        if (!strcmp(str, CORETYPES[i])) {
            return (core_type_t)i;
        }
    }
    return CORETYPE_INVALID;
}

void core_type_print(const struct variant_t* variant) {
    DEBUG_ASSERT(variant->type.kind == DATATYPE_CORE_TYPE, "Invalid variant type");

    switch (variant->type.data.builtin) {
        case CORETYPE_VOID: {
            printf(STDOUT_RED "void" STDOUT_RESET);
            break;
        }
        case CORETYPE_BOOL: {
            printf(STDOUT_RED "%s" STDOUT_RESET, BOOL2STR(variant->value.boolean));
            break;
        }
        case CORETYPE_CHAR: {
            printf(STDOUT_RED "'");
            print_char_unescaped(variant->value.character);
            printf("'" STDOUT_RESET);
            break;
        }
        case CORETYPE_STR: {
            printf(STDOUT_RED "\"" );
            print_str_unescaped(variant->value.literal.chars);
            printf("\"" STDOUT_RESET);
            break;
        }

        case CORETYPE_I64: {
            printf(STDOUT_RED "%li" STDOUT_RESET, variant->value.signed_integer);
            break;
        }
        case CORETYPE_U64: {
            printf(STDOUT_RED "%lu" STDOUT_RESET, variant->value.unsigned_integer);
            break;
        }

        
        case CORETYPE_F64: {
            printf(STDOUT_RED "%f" STDOUT_RESET, variant->value.real);
            break;
        }

        default: {
            PANIC("invalid core type %i", variant->type.data.builtin);
            break;
        }
    }

}
