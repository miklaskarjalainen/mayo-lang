#include <stb/stb_ds.h>
#include <stdio.h>

#include "semantics.h"
#include "parser.h"

#include "common/arena.h"
#include "common/error.h"
#include "common/sym_table.h"
#include "common/utils.h"
#include "compile_error.h"

#define ANALYZER_ERROR(position, ...) do { if (position.filepath ) { PRINT_ERROR_IN_FILE(position, __VA_ARGS__); LONGJUMP(1); } else { PANIC(__VA_ARGS__); } } while(0)
#define ANALYZE_BODY(body) do {             \
    const size_t Length = arrlenu(body);    \
    for (size_t i = 0; i < Length; i++) {   \
        _analyze_node(body[i], analyzer);   \
    }                                       \
} while(0)

typedef struct semantic_analyzer_t {
    sym_table_t functions, variables, structs;
} semantic_analyzer_t;

static void _analyze_node(ast_node_t* node, semantic_analyzer_t* analyzer) {
    switch (node->kind) {
        case AST_TRANSLATION_UNIT: {
            ANALYZE_BODY(node->data.translation_unit.body);
        }

        case AST_FUNCTION_DECLARATION: { 
            {
                ast_node_t* fn = sym_table_get(&analyzer->functions, node->data.function_declaration.name);
                if (fn) {
                    ANALYZER_ERROR(node->position, "Function '%s' is defined more than once!", node->data.function_declaration.name);
                }
            }
            sym_table_insert(&analyzer->functions, node->data.function_declaration.name, node);
            break;
        }

        default: {
            break;
        }
    }
    
}

void semantic_analysis(ast_node_t* node) {
    semantic_analyzer_t analyzer = { 0 };
    sym_table_init(&analyzer.functions);
    sym_table_init(&analyzer.variables);
    sym_table_init(&analyzer.structs);

    _analyze_node(node, &analyzer);
}

