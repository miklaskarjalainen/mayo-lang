#include <stdlib.h>

#include "arena.h"
#include "error.h"

arena_t arena_new(size_t size) {
    arena_t arr = {
        .size = 0,
        .capacity = size,
        .data = NULL,
        .child = NULL
    };
    arena_init(&arr, size);
    return arr; 
}

void arena_init(arena_t* arena, size_t size) {
    DEBUG_ASSERT(arena, "arena is null");

    arena->size = 0;
    arena->capacity = size;
    arena->child = NULL;
    arena->data = malloc(size);
    RUNTIME_ASSERT(arena->data, "could not allocate memory for arena!"); 
}

void arena_free(arena_t* arena) {
    DEBUG_ASSERT(arena, "arena is null");
    free(arena->data);
    arena->data = NULL;
    arena->capacity = 0;
    arena->size = 0;

    if (arena->child) {
        arena_free(arena->child);
        free(arena->child);
    }
}

void arena_reset(arena_t* arena) {
    DEBUG_ASSERT(arena, "arena is null");
    arena->capacity = 0;
    arena->size = 0;
    arena_reset(arena->child);
}

void* arena_alloc(arena_t* arena, size_t size) {
    DEBUG_ASSERT(arena, "arena is null");

    // Requested size does not fit in the arena!
    if (arena->size + size > arena->capacity) {
        // If not already, create a child arena which is the same capacity. 
        if (!arena->child) {
            arena->child = malloc(sizeof(arena_t));

            // Fail safe: if the requested size is larger than the capacity of 1 arena, use the requested size as the capacity.
            size_t WantedCapacity = arena->capacity >= size ? arena->capacity : size;
            arena_init(arena->child, WantedCapacity);
        }

        // And use it to allocate the memory.
        return arena_alloc(arena->child, size);
    }

    uint8_t* ptr = &arena->data[arena->size]; 
    arena->size += size;
    return ptr;
}
