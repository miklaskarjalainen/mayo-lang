#include <stdio.h>
#include <string.h>

#include "../common/error.h"
#include "variant.h"

// e.g mutable i32 pointer -> "mut i32*" 
void datatype_print(const datatype_t* datatype) {
    RUNTIME_ASSERT(datatype, "DATATYPE IS NULL");    

    switch (datatype->kind) {
        case DATATYPE_NULL: {
            printf("<null-type>");
            break;
        }

        case DATATYPE_POINTER: {
            const datatype_t* Inner = datatype->base;
            if (Inner != NULL) {
                datatype_print(Inner);
            }
            putchar('*');
            break;
        }

        case DATATYPE_ARRAY: {
            const size_t ArrayLen = datatype->array_size;
            const datatype_t* Inner = datatype->base;
            if (Inner != NULL) {
                datatype_print(Inner);
            }
            printf("[%zu]", ArrayLen);
            break;
        }

        case DATATYPE_PRIMITIVE: {
            printf("%s", datatype->typename);
            break;
        }

        default: {
            PANIC("undefined type?");
            break;
        }
    }


}

const datatype_t* datatype_underlying_type(const datatype_t* type) {
    DEBUG_ASSERT(type, "type is null");

    switch (type->kind) {
        case DATATYPE_ARRAY:
        case DATATYPE_POINTER: {
            return datatype_underlying_type(type->base);
        }

        default: {
            return type;
        }
    }
}

bool datatype_cmp(const datatype_t* lhs, const datatype_t* rhs) {
    if (lhs == rhs) {
        return true;
    }
    if (lhs->kind != rhs->kind) {
        return false;
    }

    switch (lhs->kind) {
        case DATATYPE_POINTER: {
            return datatype_cmp(lhs->base, rhs->base);
        }
        case DATATYPE_ARRAY: {
            if (lhs->array_size != rhs->array_size) {
                return false;
            }
            return datatype_cmp(lhs->base, lhs->base);
        }
        case DATATYPE_PRIMITIVE: {
            return strcmp(lhs->typename, rhs->typename) == 0;
        }

        default: {
            PANIC("not implemented!");
        }
    }

    return false;
}
