#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"


bool is_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

bool is_dir(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

char* read_file_contents(const char* fpath) {
    // Open with error handling
    FILE *f = fopen(fpath, "rb");
    if (f == NULL) {
        printf("Could not fopen path '%s'\n", fpath);
        return NULL;
    }
    if (is_dir(fpath)) {
        printf("'%s' is a directory and not a file!\n", fpath);
        fclose(f);
        return NULL;
    }
    if (!is_file(fpath)) {
        printf("'%s' is not a file!\n", fpath);
        fclose(f);
        return NULL;
    }

    // Seek the end of file
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    // Create a buffer and read the file contents into the buffer.
    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    // Terminate with null
    string[fsize] = '\0';
    return string;
}

bool write_file_contents(const char* fpath, const char* contents) {
    FILE* f = fopen(fpath, "w");
    if (f == NULL) {
        printf("could not create file handle file '%s'", fpath);
        return false;
    }
    const size_t Len = strlen(contents);
    fwrite(contents, sizeof(*contents), Len, f);
    fclose(f);
    return true;
}

size_t count_digits(size_t num) {
    size_t count = 1;
    while (num > 9) {
        num /= 10;
        count++;
    }
    return count;
}

void print_spaces(size_t space_count) {
    for (size_t i = 0; i < space_count; i++) {
        printf(" ");
    }
}
