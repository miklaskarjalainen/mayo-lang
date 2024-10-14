#include <stdlib.h>
#include <string.h>

#include "arena.h"
#include "error.h"

static arena_t* arena_get_child(const arena_t* arena) {
    return (arena_t*)(arena->data);
}

arena_t arena_new(size_t size) {
    arena_t arr = {
        .size = 0,
        .capacity = 0,
        .data = NULL,
    };
    arena_init(&arr, size);
    return arr; 
}

void arena_init(arena_t* arena, size_t size) {
    DEBUG_ASSERT(arena, "arena is null");

    // First bytes of data stores another arena.
    // So if we overrun this arena we can start using that one.
    arena->size = sizeof(arena_t);
    arena->capacity = size+sizeof(arena_t);
    arena->data = malloc(arena->capacity);

    {
        arena_t* child = arena_get_child(arena);
        child->capacity = 0;
        child->size = 0;
        child->data = NULL;
    }

    RUNTIME_ASSERT(arena->data, "could not allocate memory for arena!"); 
}

void arena_free(arena_t* arena) {
    DEBUG_ASSERT(arena, "arena is null");
    if (arena->data) {
        arena_free(arena_get_child(arena));
        free(arena->data);
    }
    arena->data = NULL;
    arena->capacity = 0;
    arena->size = 0;
}

void arena_reset(arena_t* arena) {
    DEBUG_ASSERT(arena, "Arena is null. Passed null as parameter!");
    if (arena->data) {
        arena_reset(arena_get_child(arena));
    }
    arena->size = sizeof(arena_t);
}

void* arena_alloc(arena_t* arena, size_t size) {
    DEBUG_ASSERT(arena, "Arena is null. Passed null as parameter!");
    DEBUG_ASSERT(arena->data, "Data is null! Arena not initialized!");

    // Requested size does not fit in the arena!
    if (arena->size + size > arena->capacity) {
        // If not already, initialize child arena if not already initialized.
        arena_t* child = arena_get_child(arena);
        if (!child->data) {
            // Fail safe: if the requested size is larger than the capacity of 1 arena, use the requested size as the capacity for the child arena.
            const size_t RealUsableCapacity = arena->capacity - sizeof(arena_t); // Child arena is also stored in data.
            const size_t ChildCapacity = 
                RealUsableCapacity >= size ?
                    RealUsableCapacity : 
                    size;
            
            arena_init(child, ChildCapacity);
        }

        // And use it to allocate the memory.
        return arena_alloc(child, size);
    }

    uint8_t* ptr = &arena->data[arena->size]; 
    arena->size += size;
    return ptr;
}

void* arena_alloc_zeroed(arena_t* arena, size_t size) {
    void* data = arena_alloc(arena, size);
    memset(data, 0, size);
    return data;
}
