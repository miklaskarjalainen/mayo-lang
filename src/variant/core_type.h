#ifndef MYLANG_CORE_TYPE_H
#define MYLANG_CORE_TYPE_H

typedef enum core_type_t {
    CORETYPE_INVALID = 0,

    /* Simple */
    CORETYPE_VOID,
    CORETYPE_BOOL,
    CORETYPE_CHAR,

    /* Integers */
    CORETYPE_I32,
    CORETYPE_I64,
    CORETYPE_U32,
    CORETYPE_U64,
    CORETYPE_USIZE,

    /* Floats */
    CORETYPE_F32,
    CORETYPE_F64,

    CORETYPE_STR,

    CORETYPE_COUNT
} core_type_t;

struct variant_t;

unsigned int core_type_size(core_type_t core_type);
const char* core_type_to_str(core_type_t core_type);
core_type_t core_type_from_str(const char* str);
void core_type_print(const struct variant_t* variant);

#endif
