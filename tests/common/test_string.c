#include <stdlib.h>
#include <string.h>

#include <criterion/criterion.h>

#include "../../src/common/string.h"
#include "../../src/common/error.h"

Test(string_tests, string_new_n_delete) {
    string_t str = string_new();
    cr_assert(str.chars != NULL);
    cr_assert(str.length == 0);

    string_delete(&str);
    cr_assert(str.chars == NULL);
    cr_assert(str.length == 0);
    cr_assert(str.capacity == 0);
}

Test(string_tests, string_resize) {
    string_t str = string_with_capacity(1);

    // grow 
    string_resize(&str, 5);
    cr_assert(str.capacity == 5);
    
    // grow past 5, and shrink it back.
    string_concat_str(&str, "four i sure hope that 'four' is the string that is left after");
    string_resize(&str, 5);
    cr_assert(str.capacity == 5);
    cr_assert_str_eq(str.chars, "four");

    string_delete(&str);
}

Test(string_tests, string_with_capacity) {
    string_t str = string_with_capacity(69);

    cr_assert(str.chars != NULL);
    cr_assert_eq(str.length, 0);
    cr_assert_eq(str.capacity, 69);

    string_delete(&str);
}

Test(string_tests, string_from) {
    const char* MyStr = "this is a text text";
    string_t str = string_from(MyStr);

    cr_assert_eq(strlen(MyStr), str.length);
    cr_assert_str_eq(str.chars, MyStr);

    string_delete(&str);
}

Test(string_tests, string_clear) {
    string_t str = string_from("Hello World");
    const size_t PrevCapacity = str.capacity;
    string_clear(&str);

    cr_assert_eq(str.capacity, PrevCapacity);
    cr_assert_eq(str.length, 0);
    cr_assert(string_is_empty(&str));;

    string_delete(&str);
}


Test(string_tests, string_move_str) {
    const char* HelloWorld = "Hello World";

    // Create char* with "Hello World" in it
    const unsigned int Length = strlen(HelloWorld);
    char* my_str = malloc(sizeof(char) * (Length + 1));
    memcpy(my_str, HelloWorld, sizeof(char) * Length);

    string_t str = string_move_str(my_str);

    cr_assert_str_eq(str.chars, "Hello World");
    cr_assert_eq(str.chars, my_str);
    cr_assert_eq(str.capacity, Length+1);
    cr_assert_eq(str.length, Length);

    string_delete(&str);
}



Test(string_tests, string_push_n_pop) {
    string_t str = string_new();
    string_push(&str, '1');
    string_push(&str, '2');
    string_push(&str, '4');

    cr_assert_eq(string_pop(&str), '4');
    cr_assert_eq(string_pop(&str), '2');
    cr_assert_eq(string_pop(&str), '1');
}

Test(string_tests, stack_string_pop_assertion, .exit_code = ASSERT_EXIT_CODE) {
    string_t str = string_new();
    
    const char InitCount = 25;

    // Push
    for (char i = 0; i < InitCount; i++) {
        string_push(&str, i+1);
    }

    // Pop
    for (char i = 0; i < InitCount; i++) {
        char c = string_pop(&str);
        UNUSED(c);
    }
    
    // Assert
    fclose(stdout);
    string_pop(&str);
}

Test(string_tests, string_push_basic) {
    string_t str = string_from("my_cool_string\0");
    cr_assert_str_eq(str.chars, "my_cool_string");
    string_push(&str, '_');
    string_push(&str, 'c');
    cr_assert_str_eq(str.chars, "my_cool_string_c");
}

Test(string_tests, string_concat) {
    string_t str1 = string_from("Hello ");
    string_t str2 = string_from("World!");
    string_concat(&str1, &str2);
    cr_assert_str_eq(str1.chars, "Hello World!");
}

Test(string_tests, string_concat_str) {
    string_t str1 = string_from("Hello ");
    const char* str2 = "World!";
    string_concat_str(&str1, str2);
    cr_assert_str_eq(str1.chars, "Hello World!");
}

Test(string_tests, string_substr) {
    string_t str = string_from("Hello, World!");
    string_t sub = string_substr(&str, 7, 5);
    cr_assert_str_eq(sub.chars, "World");
}

Test(string_tests, string_substr_str) {
    const char* Str = "Hello, World!";
    string_t sub = string_substr_str(Str, 7, 6);
    cr_assert_str_eq(sub.chars, "World!");
}

Test(string_tests, string_find) {
    const string_t Str = string_from("Hello, World!");
    size_t location1 = string_find(&Str, 'W', 0);
    cr_assert_eq(location1, 7);

    size_t location2 = string_find(&Str, 'W', 8);
    cr_assert_eq(location2, STRING_NPOS);
}



