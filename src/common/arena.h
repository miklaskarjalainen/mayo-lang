#ifndef COMMON_ARENA_H
#define COMMON_ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct arena_t {
    size_t size;
    size_t capacity;
    uint8_t* data;

    struct arena_t* child; // If the arena's capacity was overran then a child is created and used.
} arena_t;

// Initialize the arena allocator
arena_t arena_new(size_t size);
void arena_init(arena_t* arena, size_t size);
void arena_free(arena_t* arena);
void arena_reset(arena_t* arena);
void* arena_alloc(arena_t* arena, size_t size);

#endif
