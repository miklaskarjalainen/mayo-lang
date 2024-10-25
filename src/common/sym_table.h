#ifndef SYM_TABLE_H
#define SYM_TABLE_H

/*  
    This is basically just a hashmap.

    My implementation:    
        Index = hash(key) % MAX_NUM
        if table[Index].sym == NULL {
            insert:
            table[Index] = this
            return;
        }
        if !strcmp(table[Index].sym, key) {
            overwrite:
            table[Index].other = this
            return;
        }
        collision:
        table[Index].other = this
*/

#include "arena.h"

#define SYM_TABLE_CAPACITY 32

struct ast_node_t;

//? TBH just a dynamic array would probably be sufficient, since most scopes don't have many identifiers.
typedef struct symbol_t {
    const char* key;
    struct ast_node_t* data;
    struct symbol_t* other;
} symbol_t;

typedef struct sym_table_t {
    arena_t arena;
    symbol_t head[SYM_TABLE_CAPACITY];
    struct sym_table_t* parent; // If key not found in this table, tries to search from here next. Set by user.
} sym_table_t;

void sym_table_init(sym_table_t* table);
void sym_table_insert(sym_table_t* table, const char* key, struct ast_node_t* data);
void* sym_table_get(const sym_table_t* table, const char* key);
void sym_table_clear(sym_table_t* table);
void sym_table_cleanup(sym_table_t* table);


#endif
