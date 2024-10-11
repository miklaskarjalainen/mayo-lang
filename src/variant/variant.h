#ifndef MYLANG_VARIANT_H
#define MYLANG_VARIANT_H

#include "../common/string.h"
#include "core_type.h"

typedef enum datatype_kind {
    DATATYPE_NULL,
    DATATYPE_CORE_TYPE,
    DATATYPE_POINTER,
    DATATYPE_ARRAY,
    DATATYPE_STRUCT,
} datatype_kind;

typedef struct datatype_t {
    union {
        core_type_t builtin;
        const char* typename;
        struct datatype_t* pointer;
        struct {
            size_t size;
            struct datatype_t* inner;
        } array;
    } data;

    datatype_kind kind;
} datatype_t;

typedef struct variant_t {
    datatype_t type;
    union {
        char character;
        int64_t signed_integer;
        uint64_t unsigned_integer;
        double real;
        bool boolean;
        string_t literal;
        void* ptr;
    } value;
} variant_t;

/* Datatype*/
datatype_t datatype_new(datatype_kind kind);
void datatype_print(const datatype_t* datatype);
const datatype_t* datatype_underlying_type(const datatype_t* type); // i32* -> i32, i32*[2] -> i32, i32[10] -> i32

/* Variant */
variant_t variant_new(datatype_kind kind);
variant_t variant_core(core_type_t core);
void variant_cleanup(variant_t* variant);
void variant_print(const variant_t* variant);

void variant_add(variant_t* dst, const variant_t* other);
void variant_sub(variant_t* dst, const variant_t* other);
void variant_mul(variant_t* dst, const variant_t* other);
void variant_div(variant_t* dst, const variant_t* other);

bool variant_is_builtin(const variant_t* variant);
bool variant_is_pointer(const variant_t* variant);
bool variant_is_valid(const variant_t* variant);
bool variant_is_number(const variant_t* variant);
bool variant_is_integer(const variant_t* variant);
bool variant_is_builtin_type(const variant_t* variant, core_type_t builtin);

int64_t variant_as_integer(const variant_t* variant); // casts booleans floats and integers to an integer.

const char* variant_get_cstr(const variant_t* variant);
bool variant_get_bool(const variant_t* variant);
int64_t variant_get_int(const variant_t* variant);
uint64_t variant_get_uint(const variant_t* variant);
double variant_get_real(const variant_t* variant);
void* variant_get_ptr(const variant_t* variant);


#endif
