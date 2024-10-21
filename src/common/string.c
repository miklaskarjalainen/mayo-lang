#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "string.h"
#include "error.h"

#define STRING_INIT_CAPACITY 4
#define STRING_GROW(old_capacity) (old_capacity+1)

string_t string_with_capacity(size_t capacity) {
    string_t str = {
        .chars = NULL,
        .length = 0,
        .capacity = 0
    };
    string_resize(&str, capacity);
    return str;
}

void string_resize(string_t* str, size_t new_capacity) {
    // Check
    if (str->capacity == new_capacity) {
        return;
    }
    RUNTIME_ASSERT(new_capacity > 0, "cannot allocate capacity of '0'");

    // Malloc new memory
    char* new_ptr = calloc(new_capacity, sizeof(char));
    RUNTIME_ASSERT(new_ptr != NULL, "could not allocate memory for string_t");
    
    // Copy the elements from the last pointer location
    if (str->chars != NULL) {
        // We're shrinking the capacity and the existing string doesn't fit in the new one.
        if (str->length >= new_capacity) {
            memcpy(new_ptr, str->chars, sizeof(char) * new_capacity);
            // Update length to be on part with the new capacity.
            str->length = new_capacity - 1;
            // insert null terminator, at the last index.
            new_ptr[str->length] = '\0';
        }
        else {
            memcpy(new_ptr, str->chars, sizeof(char) * str->length);
        }
        free(str->chars);
    }
    
    // Assign
    str->chars = new_ptr;
    str->capacity = new_capacity;
}

string_t string_new(void) {
    return string_with_capacity(STRING_INIT_CAPACITY);
}

string_t string_from(const char* other_str) {
    const unsigned int OtherLen = strlen(other_str);
    string_t str = string_with_capacity(OtherLen+1);
    memcpy((void*)str.chars, other_str, sizeof(char) * OtherLen);
    str.length = OtherLen;
    return str;
}

void string_delete(string_t* str) {
    DEBUG_ASSERT(str != NULL, "str is NULL");
    if (str->chars != NULL) {
        free(str->chars);
        str->chars = NULL;
    }
    str->capacity = 0;
    str->length = 0;
}

string_t string_move_str(char* other_str) {
    const unsigned int Len = strlen(other_str);
    string_t str = {
        .chars = other_str,
        .length = Len,
        .capacity = Len+1
    };
    return str;
}

void string_clear(string_t* str) {
    memset(str->chars, '\0', sizeof(char) * str->capacity);
    str->length = 0;
}

bool string_is_empty(const string_t* str) {
    return str->length == 0;
}

void string_push(string_t* str, char c) {
    // '-1' accounts for null-terminator.
    DEBUG_ASSERT(str->length <= (str->capacity-1), "capacity(%zu) is less than length(%zu)", str->capacity, str->length);
    // Needing for realloc
    if (str->length == (str->capacity-1)) {
        string_resize(str, STRING_GROW(str->capacity));
    }
    str->chars[str->length++] = c;
}

char string_pop(string_t* str) {
    RUNTIME_ASSERT(!string_is_empty(str), "trying pop on a empty string");
    str->length -= 1;
    char c = str->chars[str->length];
    str->chars[str->length] = '\0';
    return c;
}

void string_concat(string_t* dst, const string_t* other) {
    DEBUG_ASSERT(other != NULL, "'other' cannot be null");
    string_concat_str(dst, other->chars);
}

void string_concat_str(string_t* dst, const char* other) {
    DEBUG_ASSERT(other != NULL, "'other' cannot be null");

    const size_t LenOther = (size_t)strlen(other);
    const size_t CombinedLen = dst->length + LenOther; 

    // Needing for realloc
    if (CombinedLen+1 > dst->capacity) {
        string_resize(dst, CombinedLen+1);
    }

    strcat(dst->chars, other);
    dst->length = CombinedLen;
}

string_t string_substr(const string_t* str, size_t pos, size_t len) {
    return string_substr_str(str->chars, pos, len);   
}

string_t string_substr_str(const char* str, size_t pos, size_t len) {
    // create a string which will hold the substring
    const unsigned int SrcStrLen = strlen(str);    
    const size_t SubStrMaxIdx = pos+len;   
    DEBUG_ASSERT(SubStrMaxIdx <= SrcStrLen, "substr's index is out of bounds got %zu whilst the length is %u", SubStrMaxIdx, SrcStrLen);
    string_t new_str = string_with_capacity(len+1);

    // memcpy the wanted substring
    const char* SubStrStart = &str[pos];
    memcpy(new_str.chars, SubStrStart, len);
    new_str.length = len;
    return new_str;
}

void string_reverse(string_t* str) {
    if (str->length < 2) {
        return;
    }

    const size_t Len = str->length;
    for (size_t i = 0; i < Len/2; i++) {
        char temp = str->chars[i];  
        str->chars[i] = str->chars[Len - i - 1];  
        str->chars[Len - i - 1] = temp;  
    }
}

bool string_eq(const string_t* str, const string_t* other_str) {
    return !strcmp(str->chars, other_str->chars);
}

bool string_eq_str(const string_t* str, const char* other_str) {
    return !strcmp(str->chars, other_str);
}

void string_toupper(string_t* str) {
    for (size_t i = 0; i < str->length; i++) {
        str->chars[i] = toupper(str->chars[i]);
    }
}

void string_tolower(string_t* str) {
    for (size_t i = 0; i < str->length; i++) {
        str->chars[i] = tolower(str->chars[i]);
    }
}

size_t string_find(const string_t* str, char what, size_t start) {
    for (size_t i = start; i < str->length; i++) {
        if (str->chars[i] == what) {
            return i;
        }
    }
    return STRING_NPOS;
}

void string_print_pretty(const string_t* str) {
    printf("(string_t) {\n  .chars='");
    for (size_t i = 0; i < str->capacity; i++) {
        const char C = str->chars[i];

        if (i == str->length) { putchar('['); }

        switch (C) {
        case '\0':
            putchar('.');
            break;
        case '\n':
            printf("(\\n)");
            break;
        default:
            putchar(str->chars[i]);
            break;
        }

        if (i == str->length) { putchar(']'); }
    }
    printf("',\n  .length=%zu\n  .capacity=%zu\n};\n", str->length, str->capacity);
}

string_t string_unescaped(const char* other) {
    string_t new = string_new();

    const size_t Len = strlen(other);
    for (size_t i = 0; i < Len; i++) {
        // Catch escapes
        const char c = other[i];
        switch (c) {
        case '\\': 
            string_push(&new, '\\'); string_push(&new, '\\');
            break;
        case '\b':
            string_push(&new, '\\'); string_push(&new, 'b');
            break;
        case '\t':
            string_push(&new, '\\'); string_push(&new, 't');
            break;
        case '\r':
            string_push(&new, '\\'); string_push(&new, 'r');
            break;
        case '\n':
            string_push(&new, '\\'); string_push(&new, 'n');
            break;
        case '\0':
            string_push(&new, '\\'); string_push(&new, '0');
            break;

        default:
            string_push(&new, c);
            break;
        }
    }
    return new;
}

void print_char_unescaped(char c) {
    // Catch escapes
    switch (c) {
    case '\b':
        printf(STDOUT_UNDERLINE "\\b" STDOUT_UNDERLINE_OFF);
        break;
    case '\t':
        printf(STDOUT_UNDERLINE "\\t" STDOUT_UNDERLINE_OFF);
        break;
    case '\r':
        printf(STDOUT_UNDERLINE "\\r" STDOUT_UNDERLINE_OFF);
        break;
    case '\n':
        printf(STDOUT_UNDERLINE "\\n" STDOUT_UNDERLINE_OFF);
        break;
    case '\0':
        printf(STDOUT_UNDERLINE "\\0" STDOUT_UNDERLINE_OFF);
        break;

    default:
        putchar(c);
        break;
    }
}

void print_str_unescaped(const char* str) {
    const size_t Len = strlen(str);

    for (size_t i = 0; i < Len; i++) {
        print_char_unescaped(str[i]);
    }
}

const char* int_to_str(int num) {
    static char str[21] = { 0 }; // largest 64 bit number has at max, 20 digits + 1 for null-terminator
    memset((void*)&str, 0, sizeof(str));
    sprintf((char*)&str, "%i", num);
    return (char*)&str;
}

const char* uint_to_str(unsigned int num) {
    static char str[21] = { 0 }; // largest 64 bit number has at max, 20 digits + 1 for null-terminator
    memset((void*)&str, 0, sizeof(str));
    sprintf((char*)&str, "%u", num);
    return (char*)&str;
}

bool is_integer(const char* str) {
    const size_t Len = strlen(str);
    if (Len == 0) {
        return false;
    }
    /*
    @TODO: hex & binary & octal numbers
        const bool Prefixed = (Len > 2) && str[0] == '0' && (
            str[1] == 'b' || str[1] == 'B' || str[1] == 'x' || str[1] == 'X'
        );
    */
    
    const size_t StartIdx = str[0] == '-' ? 1 : 0;
    for (size_t i = StartIdx; i < Len; i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

bool is_floating_point(const char *str) {
    const size_t Len = strlen(str);
    const size_t StartIdx = str[0] == '-' ? 1 : 0;
    
    bool had_dot = false; // Dot is only allowed to occur once.
    for (size_t i = StartIdx; i < Len; i++) {
        if (isdigit(str[i])) {
            continue;
        }
        if (str[i] == '.' && !had_dot) {
            had_dot = true;
            continue;
        }
        return false;
    }
    return true;
}

// Parses string until invalid character is found.
// Returns the amount of characters eaten
static size_t str2int(const char* str, int64_t* out) {
    const bool IsNegative = str[0] == '-';
    size_t i = (size_t)IsNegative;
    for (; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            break;
        }
        *out *= 10;
        *out += str[i] - '0';
    }

    if (IsNegative) {
        *out *= -1;
    }

    return i;
}

float str2f32(const char* str) {
    // This functions assumes that the 'str' is a valid float.
    // Parsing is done in 3 stages.
    // 1. Parse the whole numbers
    // 2. Parse the decimals
    // 3. Divide with the power of decimals parsed.
    
    int64_t all_digits = 0;
    int64_t parse_count = str2int(str, &all_digits);
    
    if (str[parse_count] == '.') {
        parse_count = str2int(&str[parse_count+1], &all_digits);
        
        // Makeshift power of
        int divider = 1;
        for (int64_t i = 0; i < parse_count; i++) {
            divider *= 10;
        }
        
        return (float)(all_digits) / (float)divider;
    }

    return (float)all_digits;
}

bool issym(char c) {
    if((c >= '0' && c <= '9') || c == '_') {
        return true;
    }
    if (isalpha(c)) {
        return true;
    }
    return false;
}
