#ifndef COMMON_ARENA_H
#define COMMON_ARENA_H

#include <stddef.h>
#include <stdint.h>

/*
    All arenas will store another arena at *data when initialized.
    e.g MEMORY [ arena_t, rest of of the allocated data ]
                   ^-- data*
*/
typedef struct arena_t {
    size_t size;
    size_t capacity;
    uint8_t* data;
} arena_t;


// Initialize the arena allocator
arena_t arena_new(size_t available_capacity);
void arena_init(arena_t* arena, size_t available_capacity);
void arena_free(arena_t* arena);
void arena_reset(arena_t* arena);

// These allocation functions are guaranteed to be non NULL
void* arena_alloc(arena_t* arena, size_t size); 
void* arena_alloc_zeroed(arena_t* arena, size_t size);

#endif
