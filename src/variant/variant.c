#include <stdio.h>
#include <string.h>

#include "../common/error.h"
#include "variant.h"

#define TYPE_BUFFER_LEN 0xFF

// Recursion not needed here, but was easier to for me to think this through.
static void _impl_datatype_to_str(const datatype_t* datatype, char* buffer, size_t* len) {
    // TODO: error handling when going past 'TYPE_BUFFER_LEN', having typenames larger than 255 will no be common though.
    switch (datatype->kind) {
        case DATATYPE_PRIMITIVE: {
            size_t TypenameLen = strlen(datatype->typename);
            char* next = buffer + *len;
            strncpy(next, datatype->typename, TypenameLen);
            *len = TypenameLen;
            break;
        }

        case DATATYPE_POINTER: {
            _impl_datatype_to_str(datatype->base, buffer, len);
            char* next = buffer + *len;
            *next = '*';
            (*len)++;
            break;
        }

        case DATATYPE_ARRAY: {
            _impl_datatype_to_str(datatype->base, buffer, len);
            char* next = buffer + *len;
            *len += sprintf(next, "[%zu]", datatype->array_size);
            break;
        }

        case DATATYPE_VARIADIC: {
            (buffer + *len)[0] = '.';
            (buffer + *len)[1] = '.';
            (buffer + *len)[2] = '.';
            *len += 3;
            break;
        }

        default: {
            PANIC("?");
            break;
        }
    }
}

const char* datatype_to_str(const datatype_t* datatype) {
    static char s_DatatypeStr[TYPE_BUFFER_LEN] = { 0 };
    memset(s_DatatypeStr, 0, sizeof(s_DatatypeStr));
    size_t size = 0;
    _impl_datatype_to_str(datatype, s_DatatypeStr, &size);
    return s_DatatypeStr;
}

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

        case DATATYPE_VARIADIC: {
            printf("...");
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
        // Pointer decay, an array can have pointer type, but pointer cannot be an array.
        // @FIXME: tbh should not be here, can be confusing.
        if (lhs->kind == DATATYPE_POINTER && rhs->kind == DATATYPE_ARRAY) {
            return datatype_cmp(lhs->base, rhs->base);
        }
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
        case DATATYPE_VARIADIC: {
            return false;
        }

        default: {
            PANIC("not implemented!");
        }
    }

    return false;
}
