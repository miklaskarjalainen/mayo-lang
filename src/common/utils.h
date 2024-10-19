#pragma once
#include <stdbool.h>
#include <stddef.h>

#if !defined(STDOUT_COLORS)
    #define STDOUT_COLORS
    #define STDOUT_RESET   "\033[0;0m"
    #define STDOUT_BLACK   "\033[30m"
    #define STDOUT_WHITE   "\033[37m"
    #define STDOUT_RED     "\033[31m"
    #define STDOUT_YELLOW  "\033[33m"
    #define STDOUT_CYAN    "\033[36m"
    #define STDOUT_GREEN   "\033[32m"
    #define STDOUT_MAGENTA "\033[35m"
    #define STDOUT_BLUE    "\033[34m"
    #define STDOUT_BOLD    "\033[1m"

    #define STDOUT_UNDERLINE "\033[4m"
    #define STDOUT_UNDERLINE_OFF "\033[24m"
    
    #define STDOUT_WARNING STDOUT_YELLOW STDOUT_BOLD
    #define STDOUT_ERROR  STDOUT_RED STDOUT_BOLD
#endif

#if 0
/* 
    Allows a wider color range, to be printed into the terminal, just here if needed at some point :^)
    https://upload.wikimedia.org/wikipedia/commons/1/15/Xterm_256color_chart.svg 
*/ 
#define TERM_FG(color_idx) "\x1b[38;5;"#color_idx"m"
#define TERM_BG(color_idx) "\x1b[48;5;"#color_idx"m"
#endif

#define ARRAY_LEN(arr) (sizeof(arr)/sizeof(arr[0]))
#define BOOL2STR(expr) ((expr) ? "true" : "false")
#define UNUSED(var) (void)(var)

#define TEST(...)                       \
    do {                                \
        printf(STDOUT_CYAN STDOUT_BOLD);\
        printf(__VA_ARGS__);            \
        printf(STDOUT_RESET "\n");      \
    } while(0)

#define INFO(...) TEST("[INFO] " __VA_ARGS__);

#define CMD(cmd)                \
    do {                        \
        TEST("[CMD] %s", cmd);  \
        int x = system(cmd);    \
        UNUSED(x);              \
    } while(0)

bool is_file(const char *path);
bool is_dir(const char *path);
char* read_file_contents(const char* fpath);
bool write_file_contents(const char* fpath, const char* contents);
size_t count_digits(size_t num);
void print_spaces(size_t space_count);
