#ifndef MAYO_VARIANT_H
#define MAYO_VARIANT_H

#include <stddef.h>
#include <stdbool.h>

typedef enum datatype_kind {
    DATATYPE_NULL = 0,
    DATATYPE_POINTER,
    DATATYPE_ARRAY,
    DATATYPE_PRIMITIVE,
} datatype_kind;

typedef struct datatype_t {
    datatype_kind kind;
    const char* typename;
    struct datatype_t* base;
    size_t array_size;
} datatype_t;

// Datatype
void datatype_print(const datatype_t* datatype);
const datatype_t* datatype_underlying_type(const datatype_t* type); // i32* -> i32, i32*[2] -> i32, i32[10] -> i32
bool datatype_cmp(const datatype_t* rhs, const datatype_t* lhs);

#endif
