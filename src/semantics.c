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
#define CALL_ON_BODY(fn, body, ...) do {    \
    const size_t Length = arrlenu(body);    \
    for (size_t i = 0; i < Length; i++) {   \
        fn(body[i], __VA_ARGS__);           \
    }                                       \
} while(0)

typedef struct semantic_analyzer_t {
    sym_table_t functions, structs;
} semantic_analyzer_t;

static void _analyze_scoped_node(ast_node_t* node, semantic_analyzer_t* analyzer, sym_table_t* variables) {
    UNUSED(analyzer);

    switch (node->kind) {
        case AST_VARIABLE_DECLARATION: {
            {
                ast_variable_declaration_t* var_decl = sym_table_get(variables, node->data.variable_declaration.name);
                if (var_decl){
                    ANALYZER_ERROR(node->position, "Variable called '%s' is already defined!", node->data.variable_declaration.name);
                }
            }
            sym_table_insert(variables, node->data.variable_declaration.name, (void*)&node->data.variable_declaration);

            break;
        }
        default: {
            break;
        }
    }
}

static void _analyze_global_node(ast_node_t* node, semantic_analyzer_t* analyzer) {
    switch (node->kind) {
        case AST_TRANSLATION_UNIT: {
            CALL_ON_BODY(_analyze_global_node, node->data.translation_unit.body, analyzer);
            break;
        }

        case AST_FUNCTION_DECLARATION: { 
            {
                ast_node_t* fn = sym_table_get(&analyzer->functions, node->data.function_declaration.name);
                if (fn) {
                    ANALYZER_ERROR(node->position, "Function '%s' is defined more than once!", node->data.function_declaration.name);
                }
            }
            sym_table_insert(&analyzer->functions, node->data.function_declaration.name, node);

            sym_table_t variables;
            sym_table_init(&variables);
            const size_t Size = arrlenu(node->data.function_declaration.args);
            for (size_t arg = 0; arg < Size; arg++) {
                const ast_variable_declaration_t* Argument = &node->data.function_declaration.args[arg]; 
                sym_table_insert(&variables, Argument->name, (void*)Argument);
            }
            CALL_ON_BODY(_analyze_scoped_node, node->data.function_declaration.body, analyzer, &variables);
            sym_table_cleanup(&variables);
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
    sym_table_init(&analyzer.structs);

    _analyze_global_node(node, &analyzer);

    sym_table_cleanup(&analyzer.functions);
    sym_table_cleanup(&analyzer.structs);
}

