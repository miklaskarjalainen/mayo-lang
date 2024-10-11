#include <stb/stb_ds.h>
#include <stdio.h>

#include "semantics.h"
#include "parser.h"
#include "variant/variant.h"

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

typedef struct global_scope_t {
    sym_table_t functions, structs;
} global_scope_t;

static bool _analyze_is_valid_type(const global_scope_t* global, const datatype_t* type) {
    const datatype_t* TrueType = datatype_underlying_type(type);

    // CORETYPE
    if (TrueType->kind == DATATYPE_CORE_TYPE) {
        return true;
    }

    // Structs
    if (TrueType->kind == DATATYPE_STRUCT) {
        ast_variable_declaration_t* var_decl = sym_table_get(&global->structs, TrueType->data.typename);
        return var_decl != NULL;
    }

    PANIC("invalid type?");
    return false;
}

static void _analyze_scoped_node(ast_node_t* node, global_scope_t* global, sym_table_t* variables) {
    UNUSED(global);

    switch (node->kind) {
        case AST_VARIABLE_DECLARATION: {
            // Already used?
            {
                ast_node_t* var_decl = sym_table_get(variables, node->data.variable_declaration.name);
                if (var_decl){
                    ANALYZER_ERROR(node->position, "Variable called '%s' is already defined!", node->data.variable_declaration.name);
                }
            }

            // Has a valid type?
            const bool IsTypeValid = _analyze_is_valid_type(global, &node->data.variable_declaration.type);
            if (!IsTypeValid) {
                const datatype_t* UnderlyingType = datatype_underlying_type(&node->data.variable_declaration.type);
                ANALYZER_ERROR(node->position, "Type '%s' is not defined!", UnderlyingType->data.typename);
            }

            sym_table_insert(variables, node->data.variable_declaration.name,node);
            break;
        }
        default: {
            break;
        }
    }
}

static void _analyze_global_node(ast_node_t* node, global_scope_t* global) {
    switch (node->kind) {
        case AST_TRANSLATION_UNIT: {
            CALL_ON_BODY(_analyze_global_node, node->data.translation_unit.body, global);
            break;
        }

        case AST_FUNCTION_DECLARATION: { 
            // Multiple definitions?
            {
                ast_node_t* fn = sym_table_get(&global->functions, node->data.function_declaration.name);
                if (fn) {
                    ANALYZER_ERROR(node->position, "Function '%s' is defined more than once!", node->data.function_declaration.name);
                }
            }
            sym_table_insert(&global->functions, node->data.function_declaration.name, node);

            // Analyze body
            sym_table_t variables;
            sym_table_init(&variables);
            const size_t Size = arrlenu(node->data.function_declaration.args);
            for (size_t arg = 0; arg < Size; arg++) {
                const ast_variable_declaration_t* Argument = &node->data.function_declaration.args[arg]; 
                sym_table_insert(&variables, Argument->name, (void*)Argument);
            }
            CALL_ON_BODY(_analyze_scoped_node, node->data.function_declaration.body, global, &variables);
            sym_table_cleanup(&variables);
            break;
        }

        case AST_STRUCT_DECLARATION: {
            const char* StructName = node->data.struct_declaration.name;
            // Multiple definitions?
            {
                ast_node_t* struct_decl = sym_table_get(&global->structs, StructName);
                if (struct_decl) {
                    ANALYZER_ERROR(node->position, "Struct '%s' is defined more than once!", StructName);
                }
            }

            // Define struct
            sym_table_insert(&global->structs, StructName, node);
            break;
        }

        default: {
            break;
        }
    }
    
}

void semantic_analysis(ast_node_t* node) {
    global_scope_t global = { 0 };
    sym_table_init(&global.functions);
    sym_table_init(&global.structs);

    _analyze_global_node(node, &global);

    sym_table_cleanup(&global.functions);
    sym_table_cleanup(&global.structs);
}

