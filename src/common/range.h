#ifndef MYLANG_RANGE_H
#define MYLANG_RANGE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct range_t {
    int64_t from, to;
    uint64_t step;
    bool reverse;
} range_t;

#endif
