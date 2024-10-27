/*
#include <criterion/criterion.h>

#include "../src/core_type.h"

Test(core_type_tests, to_str) {
    cr_expect_str_eq(core_type_to_str(CORETYPE_INVALID), "invalid_type");
    cr_expect_str_eq(core_type_to_str(CORETYPE_AUTO), "auto");

    cr_expect_str_eq(core_type_to_str(CORETYPE_BOOL), "bool");
    cr_expect_str_eq(core_type_to_str(CORETYPE_CHAR), "char");

    cr_expect_str_eq(core_type_to_str(CORETYPE_I32), "i32");
    cr_expect_str_eq(core_type_to_str(CORETYPE_I64), "i64");
    cr_expect_str_eq(core_type_to_str(CORETYPE_U32), "u32");
    cr_expect_str_eq(core_type_to_str(CORETYPE_U64), "u64");
    cr_expect_str_eq(core_type_to_str(CORETYPE_USIZE), "usize");

    cr_expect_str_eq(core_type_to_str(CORETYPE_F32), "f32");
    cr_expect_str_eq(core_type_to_str(CORETYPE_F64), "f64");
}

Test(core_type_tests, from_str) {
    // Valid matches
    cr_expect_eq(core_type_from_str("invalid_type"), CORETYPE_INVALID);
    cr_expect_eq(core_type_from_str("auto"), CORETYPE_AUTO);

    cr_expect_eq(core_type_from_str("bool"), CORETYPE_BOOL);
    cr_expect_eq(core_type_from_str("char"), CORETYPE_CHAR);

    cr_expect_eq(core_type_from_str("i32"), CORETYPE_I32);
    cr_expect_eq(core_type_from_str("i64"), CORETYPE_I64);
    cr_expect_eq(core_type_from_str("u32"), CORETYPE_U32);
    cr_expect_eq(core_type_from_str("u64"), CORETYPE_U64);
    cr_expect_eq(core_type_from_str("usize"), CORETYPE_USIZE);

    cr_expect_eq(core_type_from_str("f32"), CORETYPE_F32);
    cr_expect_eq(core_type_from_str("f64"), CORETYPE_F64);

    // Invalid matches (not real types)
    cr_expect_eq(core_type_from_str("3f2"), CORETYPE_INVALID);
    cr_expect_eq(core_type_from_str("6f4"), CORETYPE_INVALID);
}
*/
