#ifndef MYLANG_STRING_H
#define MYLANG_STRING_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define STRING_NPOS SIZE_MAX

// String "class" which is stored in the heap, and is resizable.
typedef struct string_t {
    char* chars;
    // length should always be atleast 1 less than capacity (length is not null terminator inclusive)
    // in fact, the length should ALWAYS be the index to the null-terminator.
    size_t length; 
    size_t capacity; // capacity is null terminator inclusive
} string_t;

// Everything is quaranteed to be zeroed.
string_t string_new(void);
void string_delete(string_t* str);
// The capacity has to include the null-terminator, to store "null" capacity of 5 is needed.
void string_resize(string_t* str, size_t capacity);

string_t string_from(const char* other_str);
string_t string_with_capacity(size_t capacity);
string_t string_move_str(char* other_str); // does not perform memcpy.

void string_clear(string_t* str);
bool string_is_empty(const string_t* str);
void string_push(string_t* str, char c);
char string_pop(string_t* str);
void string_concat(string_t* dst, const string_t* other);
void string_concat_str(string_t* dst, const char* other);
void string_reverse(string_t* str);

bool string_eq(const string_t* str, const string_t* other_str);
bool string_eq_str(const string_t* str, const char* other_str);

void string_toupper(string_t* str);
void string_tolower(string_t* str);

string_t string_substr(const string_t* str, size_t pos, size_t len);
string_t string_substr_str(const char* str, size_t pos, size_t len);
// Returns the location of the char, or STRING_NPOS if not found.
size_t string_find(const string_t* str, char what, size_t start);
const char* int_to_str(int num);
const char* uint_to_str(unsigned int num);

void string_print_pretty(const string_t* str);
string_t string_unescaped(const char* other);
void print_char_unescaped(char c);
void print_str_unescaped(const char* str);

bool is_integer(const char* str);
bool is_floating_point(const char* str);
bool issym(char c);
float str2f32(const char* str);

#endif
