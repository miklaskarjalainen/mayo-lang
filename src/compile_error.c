#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "common/utils.h"
#include "compile_error.h"

// print_line_number uses this to align digits which have the same amount or less digits
#define MAX_SPACE_COUNT ((int)5)

//Correctly prints (line numbers are aligned)
//* "  9 | "  
//* " 10 | "
//* [line] the number to be printed
//* [max_digit_count] how many digits does the max line count contain
static void print_line_number(size_t line) {
    // if difference is less than 0, for loop skipped so no problem. :)
    const int difference = MAX_SPACE_COUNT - count_digits(line);
    for (int i = 0; i < difference; i++) {
        putchar(' ');
    }
    printf(STDOUT_CYAN "%zu | " STDOUT_RESET, line);
}

void print_cat(const char* contents) {
    const size_t len = strlen(contents);
    size_t line = 1;

    print_line_number(line++);
    for (size_t i = 0; i < len; i++) {
        const char c = *(contents+i);
        putchar(c);
        if (c == '\n') {
            print_line_number(line++);
        }
    }

    putchar('\n');
}

void print_error_in_file(const char* fpath, size_t line, size_t column, int word_len, const char* reason, ...) {
    if (!fpath) {
        printf("error occurred at %zu:%zu, because ", line, column);
        // Draw reason for the error.
        if (reason != NULL) {
            va_list arglist;
            va_start(arglist, reason);
            vprintf(reason, arglist);
            va_end(arglist);
            printf("\n");
        }
        return;
    }
    char* file_contents = read_file_contents(fpath);
    if (!file_contents) {
        return;
    }

    // Include few lines before and after the error.
    const size_t StartLineCount  = line >= 2 ? line-2 : 0;
    const size_t TargetLine = line + 2;

    // seek to line
    char* pointer = file_contents;
    size_t cur_line = 1; 
    while (cur_line < StartLineCount) {
        const char c = *(pointer++);
        
        switch (c) {
            case '\0':
                goto out;
                break;
            case '\n': 
                cur_line++;
                break;
            default:
                break;
        }
    }

    printf(STDOUT_CYAN "--> %s:" STDOUT_BOLD "%zu:%zu:\n" STDOUT_RESET, fpath, line, column);

    print_line_number(cur_line);
    size_t cur_column = 1;
    while (*pointer != '\0') {
        const char c = *(pointer++);

        // Print the errored chars red
        if (cur_line == line && (cur_column >= column) && cur_column < (column+word_len)) {
            printf(STDOUT_BOLD STDOUT_RED);
            putchar(c);
            printf(STDOUT_RESET);
        }
        else {
            putchar(c);
        }

        cur_column++;

        //@FIXME: error  not printed if at end of file.
        if (c == '\n') {
            // Draw '^' after the erroring line.
            if (cur_line == line) {
                // Draw blanks
                print_line_number(cur_line);
                printf(STDOUT_BOLD STDOUT_RED);
                for (size_t i = 0; i < (column-1); i++) {
                    putchar(' ');
                }

                // Draw '^'
                for (int i = 0; i < word_len; i++) {
                    putchar('^');
                }

                // Draw reason for the error.
                if (reason != NULL) {
                    printf("  <--  ");
                    va_list arglist;
                    va_start(arglist, reason);
                    vprintf(reason, arglist);
                    va_end(arglist);
                }

                printf(STDOUT_RESET "\n");
            }


            if (cur_line++ >= TargetLine) {
                break;
            }
            cur_column = 1;
            print_line_number(cur_line);
        }
    }

out:
    free(file_contents);
    putchar('\n');
}
