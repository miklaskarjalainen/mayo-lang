#include <stdio.h>
#include <string.h>

#include "../common/error.h"
#include "../common/utils.h"
#include "core_type.h"
#include "variant.h"

// e.g mutable i32 pointer -> "mut i32*" 
void datatype_print(const datatype_t* datatype) {
    RUNTIME_ASSERT(datatype, "DATATYPE IS NULL");    

    switch (datatype->kind) {
        case DATATYPE_NULL: {
            printf("<null-type>");
            break;
        }

        case DATATYPE_CORE_TYPE: {
            printf("%s", core_type_to_str(datatype->data.builtin));
            break;
        }

        case DATATYPE_POINTER: {
            const datatype_t* Inner = datatype->data.pointer;
            if (Inner != NULL) {
                datatype_print(Inner);
            }
            putchar('*');
            break;
        }

        case DATATYPE_ARRAY: {
            const size_t ArrayLen = datatype->data.array.size;
            const datatype_t* Inner = datatype->data.array.inner;
            if (Inner != NULL) {
                datatype_print(Inner);
            }
            printf("[%zu]", ArrayLen);
            break;
        }

        case DATATYPE_STRUCT: {
            printf("struct %s", datatype->data.typename);
            break;
        }

        default: {
            PANIC("undefined type?");
            break;
        }
    }


}

// mut x: i32 = 10 -> "10: mut i32"
void variant_print(const variant_t* variant) {
    const datatype_t Type = variant->type;

    switch (Type.kind) {
        case DATATYPE_NULL: {
            printf("<null>: <null>");
            break;
        }

        case DATATYPE_CORE_TYPE: {
            core_type_print(variant);
            printf(": ");
            datatype_print(&Type);
            break;
        }

        case DATATYPE_POINTER: {
            printf("(%p) : ", variant->value.ptr);
            datatype_print(&Type);
            break;
        }

        default: {
            break;
        }
    }

    UNUSED(variant);
}


datatype_t datatype_new(datatype_kind kind) {
    return (datatype_t) {
        .kind = kind,
        .data = { 0 }
    };
}

variant_t variant_new(datatype_kind kind) {
    return (variant_t) {
        .type = datatype_new(kind),
        .value.ptr = NULL
    };
}

const datatype_t* datatype_underlying_type(const datatype_t* type) {
    DEBUG_ASSERT(type, "type is null");

    switch (type->kind) {
        case DATATYPE_POINTER: {
            return datatype_underlying_type(type->data.pointer);
        }
        case DATATYPE_ARRAY: {
            return datatype_underlying_type(type->data.array.inner);
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
        case DATATYPE_CORE_TYPE: {
            return lhs->data.builtin == rhs->data.builtin;
        }
        case DATATYPE_POINTER: {
            return datatype_cmp(lhs->data.pointer, rhs->data.pointer);
        }
        case DATATYPE_ARRAY: {
            if (lhs->data.array.size != rhs->data.array.size) {
                return false;
            }
            return datatype_cmp(lhs->data.array.inner, lhs->data.array.inner);
        }
        case DATATYPE_STRUCT: {
            return strcmp(lhs->data.typename, rhs->data.typename) == 0;
        }

        default: {
            PANIC("not implemented!");
        }
    }

    return false;
}

variant_t variant_core(core_type_t core) {
    variant_t v = variant_new(DATATYPE_CORE_TYPE);
    v.type.data.builtin = core;
    return v;
}

void variant_cleanup(variant_t* variant) {
    if (variant->type.kind != DATATYPE_CORE_TYPE) {
        goto out;
    }
    if (variant->type.data.builtin != CORETYPE_STR) {
        goto out;
    }

    // free needed
    string_delete(&variant->value.literal);

out:
    *variant = variant_new(DATATYPE_NULL);
}

void variant_add(variant_t* dst, const variant_t* other) {
    RUNTIME_ASSERT(dst->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");
    RUNTIME_ASSERT(other->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");

    RUNTIME_ASSERT(dst->type.data.builtin == other->type.data.builtin, "type mismatch.");

    switch (dst->type.data.builtin) {
        case CORETYPE_I32:
        case CORETYPE_I64: {
            dst->value.signed_integer += other->value.signed_integer;
            return;
        }
        case CORETYPE_U32:
        case CORETYPE_U64: {
            dst->value.unsigned_integer += other->value.unsigned_integer;
            return;
        }
        case CORETYPE_F32:
        case CORETYPE_F64: {
            dst->value.real += other->value.real;
            return;
        }

        case CORETYPE_STR: {
            string_concat(&dst->value.literal, &other->value.literal);
            return;
        }


        default: {
            PANIC("addition not supported for type %s", core_type_to_str(dst->type.data.builtin));
            break;
        }
    }
}

void variant_sub(variant_t* dst, const variant_t* other) {
    RUNTIME_ASSERT(dst->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");
    RUNTIME_ASSERT(other->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");

    RUNTIME_ASSERT(dst->type.data.builtin == other->type.data.builtin, "type mismatch.");

    switch (dst->type.data.builtin) {
        case CORETYPE_I32:
        case CORETYPE_I64: {
            dst->value.signed_integer -= other->value.signed_integer;
            return;
        }
        case CORETYPE_U32:
        case CORETYPE_U64: {
            dst->value.unsigned_integer -= other->value.unsigned_integer;
            return;
        }
        case CORETYPE_F32:
        case CORETYPE_F64: {
            dst->value.real -= other->value.real;
            return;
        }


        default: {
            PANIC("addition not supported for type %s", core_type_to_str(dst->type.data.builtin));
            break;
        }
    }
}

void variant_mul(variant_t* dst, const variant_t* other) {
    RUNTIME_ASSERT(dst->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");
    RUNTIME_ASSERT(other->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");

    RUNTIME_ASSERT(dst->type.data.builtin == other->type.data.builtin, "type mismatch.");

    switch (dst->type.data.builtin) {
        case CORETYPE_I32:
        case CORETYPE_I64: {
            dst->value.signed_integer *= other->value.signed_integer;
            return;
        }
        case CORETYPE_U32:
        case CORETYPE_U64: {
            dst->value.unsigned_integer *= other->value.unsigned_integer;
            return;
        }
        case CORETYPE_F32:
        case CORETYPE_F64: {
            dst->value.real *= other->value.real;
            return;
        }


        default: {
            PANIC("addition not supported for type %s", core_type_to_str(dst->type.data.builtin));
            break;
        }
    }
}

void variant_div(variant_t* dst, const variant_t* other) {
    RUNTIME_ASSERT(dst->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");
    RUNTIME_ASSERT(other->type.kind == DATATYPE_CORE_TYPE, "only core types supported.");

    RUNTIME_ASSERT(dst->type.data.builtin == other->type.data.builtin, "type mismatch.");
    RUNTIME_ASSERT(other->value.real != 0.0, "division by 0.");

    switch (dst->type.data.builtin) {
        case CORETYPE_I32:
        case CORETYPE_I64: {
            dst->value.signed_integer /= other->value.signed_integer;
            return;
        }
        case CORETYPE_U32:
        case CORETYPE_U64: {
            dst->value.unsigned_integer /= other->value.unsigned_integer;
            return;
        }
        case CORETYPE_F32:
        case CORETYPE_F64: {
            dst->value.real /= other->value.real;
            return;
        }


        default: {
            PANIC("addition not supported for type %s", core_type_to_str(dst->type.data.builtin));
            break;
        }
    }
}

bool variant_is_builtin(const variant_t* variant) {
    return variant->type.kind == DATATYPE_CORE_TYPE;
}

bool variant_is_pointer(const variant_t* variant) {
    return variant->type.kind == DATATYPE_POINTER;
}

bool variant_is_valid(const variant_t* variant) {
    return variant->type.kind != DATATYPE_NULL;
}

bool variant_is_integer(const variant_t* variant) {
    if (!variant_is_builtin(variant)) { return false; }
    
    switch (variant->type.data.builtin) {
        case CORETYPE_CHAR: 
        case CORETYPE_I32:
        case CORETYPE_I64: 
        case CORETYPE_U32:
        case CORETYPE_U64: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool variant_is_number(const variant_t* variant) {
    if (!variant_is_builtin(variant)) { return false; }
    
    switch (variant->type.data.builtin) {
        case CORETYPE_BOOL:
        case CORETYPE_CHAR: 
        case CORETYPE_I32:
        case CORETYPE_I64: 
        case CORETYPE_U32:
        case CORETYPE_U64: 
        case CORETYPE_F32: 
        case CORETYPE_F64: {
            return true;
        }
        default: {
            return false;
        }
    }
}

bool variant_is_builtin_type(const variant_t* variant, core_type_t builtin) {
    if (!variant_is_builtin(variant)) {
        return false;
    }
    return variant->type.data.builtin == builtin;
}

int64_t variant_as_integer(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_builtin(variant), "only builtins supported at this point");
    switch (variant->type.data.builtin) {
        case CORETYPE_BOOL: {
            return (int64_t)variant->value.boolean;
        }
        case CORETYPE_CHAR: {
            return (int64_t)variant->value.character;
        }
        case CORETYPE_I32:
        case CORETYPE_I64: {
            return variant->value.signed_integer;
        }
        case CORETYPE_U32:
        case CORETYPE_U64: {
            return (int64_t)variant->value.unsigned_integer;
        }
        case CORETYPE_F32: 
        case CORETYPE_F64: {
            return (int64_t)variant->value.real;
        }
        default: {
            PANIC("cannot cast non number to an integer");
            break;
        }
    }

    return 0;
}

const char* variant_get_cstr(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_builtin_type(variant, CORETYPE_STR), "not valid type");
    return variant->value.literal.chars;
}

bool variant_get_bool(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_builtin_type(variant, CORETYPE_BOOL), "not valid type");
    return variant->value.boolean;
}

int64_t variant_get_int(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_builtin_type(variant, CORETYPE_I64), "not valid type");
    return variant->value.signed_integer;
}

uint64_t variant_get_uint(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_builtin_type(variant, CORETYPE_U64), "not valid type");
    return variant->value.unsigned_integer;
}

double variant_get_real(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_builtin_type(variant, CORETYPE_F64), "not valid type");
    return variant->value.real;
}

void* variant_get_ptr(const variant_t* variant) {
    RUNTIME_ASSERT(variant_is_pointer(variant), "not valid type");
    return variant->value.ptr;
}
