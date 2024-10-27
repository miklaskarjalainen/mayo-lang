#include <criterion/criterion.h>

#include "common/arena.h"
#include "criterion/internal/assert.h"

/*

// Initialize the arena allocator
arena_t arena_new(size_t available_capacity);
void arena_init(arena_t* arena, size_t available_capacity);
void arena_free(arena_t* arena);
void arena_reset(arena_t* arena);

// These allocation functions are guaranteed to be non NULL
void* arena_alloc(arena_t* arena, size_t size); 
void* arena_alloc_zeroed(arena_t* arena, size_t size);
*/

Test(arena_tests, arena_init) {
    const size_t WantedCapacity = 0xFF;

    {
        arena_t arena = arena_new(WantedCapacity);

        const size_t AvailableCapacity = arena.capacity - arena.size;
        cr_expect(AvailableCapacity >= WantedCapacity);
        cr_expect(arena.data != NULL);

        arena_free(&arena);
    }

    {
        arena_t arena;
        arena_init(&arena, WantedCapacity);
        
        const size_t AvailableCapacity = arena.capacity - arena.size;
        cr_expect(AvailableCapacity >= WantedCapacity);
        cr_expect(arena.data != NULL);

        arena_free(&arena);
    }
}

Test(arena_tests, arena_basic_allocation) {
    arena_t arena = arena_new(sizeof(int32_t) * 5);

    int32_t* n1 = arena_alloc(&arena, sizeof(int32_t));
    *n1 = 69420;
    int16_t* n2 = arena_alloc(&arena, sizeof(int16_t));
    *n2 = 0x1234;
    uint16_t* n3 = arena_alloc(&arena, sizeof(uint16_t));
    *n3 = 0xBEEF;
    int64_t* n4 = arena_alloc_zeroed(&arena, sizeof(int64_t));
    int32_t* n5 = arena_alloc_zeroed(&arena, sizeof(int32_t));

    cr_expect(*n1 == 69420);
    cr_expect(*n2 == 0x1234);
    cr_expect(*n3 == 0xBEEF);
    cr_expect(*n4 == 0);
    cr_expect(*n5 == 0);

    arena_free(&arena);
}

Test(arena_tests, arena_allocation_zeroed) {
    const int32_t Cmp[0xFF] = { 0 }; 

    arena_t arena = arena_new(sizeof(Cmp));
    int32_t* n1 = arena_alloc_zeroed(&arena, sizeof(Cmp));

    cr_expect(memcmp(n1, &Cmp, sizeof(Cmp)) == 0);

    arena_free(&arena);
}

Test(arena_tests, arena_overflow) {
    arena_t arena = arena_new(sizeof(char) * 32);

    {
        char* block = arena_alloc(&arena, 14);
        strcpy(block, "Hello, World!"); // 14-bytes
        cr_expect_str_eq(block, "Hello, World!");
    }
    {
        char* block = arena_alloc(&arena, 14);
        strcpy(block, "Hello, World!"); // 14-bytes
        cr_expect_str_eq(block, "Hello, World!");
    }

    {
        // 28 of 32 bytes used (4 bytes left.) Let's ask more than that. :)
        // The arena should create a sub-arena into itself and use that to allocate when overflown.
        
        // Check that sub arena has not yet allocated.
        arena_t* sub_arena = (arena_t*)arena.data;
        cr_expect(sub_arena->data == NULL);

        // Asking more memory that the arena can hold should now use the sub arena.
        char* block = arena_alloc(&arena, 10);
        strcpy(block, "Hello, World!"); // 14-bytes
        cr_expect_str_eq(block, "Hello, World!");

        // Sub arena allocated it's own chunk.
        cr_expect(sub_arena->data != NULL);
    }


    arena_free(&arena);
}
