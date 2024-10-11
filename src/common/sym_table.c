#include <string.h>
#include <stdint.h>

#include "sym_table.h"
#include "error.h"
#include "utils.h"

#ifdef NDEBUG
#define TABLE_ARENA_CAPACITY 4096
#else
#define TABLE_ARENA_CAPACITY 16
#endif

static uint32_t _hash_str(const char* str) {
    // TODO: actually make this good. It cannot use strlen, because having a loop in this would be slow.
    DEBUG_ASSERT(str, "str is nullptr");
    uint32_t hash = str[0];
    return hash;
}

void sym_table_init(sym_table_t* table) {
    arena_init(&table->arena, TABLE_ARENA_CAPACITY);
    sym_table_clear(table);
    table->parent = NULL;
}

void sym_table_insert(sym_table_t* table, const char* key, void* data) {
    const uint32_t Index = _hash_str(key) % ARRAY_LEN(table->head);

    symbol_t* sym = &table->head[Index];
     
    const size_t IterLimit = 8;
    for (size_t i = 0; i < IterLimit; i++) {
        // empty use this
        if (!sym->key) {
            sym->key = key;
            sym->data = data;
            return;
        }

        // overwrite
        if (strcmp(sym->key, key) == 0) {
            sym->data = data;
            return;
        }

        // collision
        if (!sym->other) {
            sym->other = arena_alloc_zeroed(&table->arena, sizeof(symbol_t));
        }
        sym = sym->other;
    }

    PANIC("Iteration limit reached!");
}

void* sym_table_get(const sym_table_t* table, const char* key) {
    const uint32_t Index = _hash_str(key) % ARRAY_LEN(table->head);
    const symbol_t* sym = &table->head[Index];

    const size_t IterLimit = 8;
    for (size_t i = 0; i < IterLimit; i++) {
        // Index doesn't exist
        if (!sym->key) {
            if (table->parent) {
                return sym_table_get(table->parent, key);
            }
            return NULL;
        }

        // Key existed
        if (strcmp(sym->key, key) == 0) {
            return sym->data;
        }

        // collision
        if (!sym->other) {
            return NULL;
        }
        sym = sym->other;
    }

    PANIC("Iteration limit reached!");
    return NULL;
}

void sym_table_clear(sym_table_t* table) {
    memset(table->head, 0, sizeof(table->head)); 
    arena_reset(&table->arena);
}

void sym_table_cleanup(sym_table_t* table) {
    arena_free(&table->arena);
}
