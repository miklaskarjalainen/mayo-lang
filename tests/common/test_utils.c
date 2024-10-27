#include <criterion/criterion.h>
#include "common/utils.h"

#define expect_digits(eval, expect) { \
    const size_t got = count_digits(eval); \
    cr_expect_eq(got, (size_t)expect, "Expected '%u' got '%zu' instead :^(!", expect, got); \
}

Test(utils_tests, count_digits) {
    expect_digits(0, 1);
    expect_digits(1, 1);
    expect_digits(2, 1);
    expect_digits(3, 1);
    expect_digits(4, 1);

    expect_digits(10, 2);
    expect_digits(15, 2);
    expect_digits(18, 2);
    expect_digits(10, 2);
    expect_digits(25, 2);
    expect_digits(48, 2);
    expect_digits(88, 2);
    expect_digits(99, 2);
    
    expect_digits(126, 3);
    expect_digits(1534, 4);
    expect_digits(54362, 5);
    expect_digits(889245, 6);
}
