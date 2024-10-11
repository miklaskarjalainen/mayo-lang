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

static const datatype_t* _analyze_expression(const global_scope_t* global, const sym_table_t* variables, ast_node_t* expr) {
    static const datatype_t BoolType = {
        .kind = DATATYPE_CORE_TYPE,
        .data.builtin = CORETYPE_BOOL
    };

    switch (expr->kind) {
        case AST_CONST_VALUE: {
            return &expr->data.constant.type;
        }

        case AST_GET_VARIABLE: {
            ast_variable_declaration_t* var_decl = sym_table_get(variables, expr->data.literal);
            if (!var_decl) {
                ANALYZER_ERROR(expr->position, "No variable called '%s' exists, used in expression!", expr->data.literal);
            }

            return &var_decl->type;
        }

        case AST_FUNCTION_CALL: {
            // TODO: ARGS

            ast_node_t* func_decl = sym_table_get(&global->functions, expr->data.literal);
            if (!func_decl) {
                ANALYZER_ERROR(expr->position, "Function called '%s' does not exists, used in expression!", expr->data.literal);
            }

            DEBUG_ASSERT(func_decl->kind == AST_FUNCTION_DECLARATION, "?");
            return &func_decl->data.function_declaration.return_type;
        }

        case AST_BINARY_OP: {
            const datatype_t* Lhs = _analyze_expression(global, variables, expr->data.binary_op.left);
            const datatype_t* Rhs = _analyze_expression(global, variables, expr->data.binary_op.right);
            if (!datatype_cmp(Lhs, Rhs)) {
                ANALYZER_ERROR(expr->position, "Type mismatch!");
            }

            switch (expr->data.binary_op.operation) {
                case BINARY_OP_NOT_EQUAL:
                case BINARY_OP_EQUAL: {
                    return &BoolType;
                }

                default: {
                    return Lhs;
                }
            }
        }

        default: {
            PANIC("not implemented for type %u", (uint32_t)expr->kind);
        }
    }
    
}

static void _analyze_scoped_node(ast_node_t* node, global_scope_t* global, sym_table_t* variables) {
    if (!node) {
        return;
    }

    UNUSED(global);

    switch (node->kind) {
        case AST_VARIABLE_DECLARATION: {
            const char* VarName = node->data.variable_declaration.name;

            // Already used?
            {
                ast_variable_declaration_t* var_decl = sym_table_get(variables, VarName);
                if (var_decl){
                    ANALYZER_ERROR(node->position, "Variable called '%s' is already defined!", VarName);
                }
            }

            // Has a valid type?
            const datatype_t* VarType = &node->data.variable_declaration.type;
            const bool IsTypeValid = _analyze_is_valid_type(global, VarType);
            if (!IsTypeValid) {
                const datatype_t* UnderlyingType = datatype_underlying_type(VarType);
                ANALYZER_ERROR(node->position, "Type '%s' is not defined!", UnderlyingType->data.typename);
            }

            // Get type of expression
            const datatype_t* ExprType = _analyze_expression(global, variables, node->data.variable_declaration.expr);
            
            // @FIXME: HACK to make i64 types work with i32 declarations. Do actual casts.
            if (
                !datatype_cmp(VarType, ExprType) &&
                !(
                    VarType->kind == DATATYPE_CORE_TYPE && ExprType->kind == DATATYPE_CORE_TYPE &&
                    VarType->data.builtin == CORETYPE_I32 && ExprType->data.builtin == CORETYPE_I64 
                )
            ) {
                ANALYZER_ERROR(node->position, "Type mismatch between declaration and expression!");
            }

            sym_table_insert(variables, VarName, &node->data.variable_declaration);
            break;
        }

        case AST_IF_STATEMENT: {
            // expr
            _analyze_expression(global, variables, node->data.if_statement.expr);

            sym_table_t if_scope = { 0 };
            sym_table_init(&if_scope);
            if_scope.parent = variables;

            // TODO: SCOPES
            CALL_ON_BODY(_analyze_scoped_node, node->data.if_statement.body, global, &if_scope);
            sym_table_clear(&if_scope);
            CALL_ON_BODY(_analyze_scoped_node, node->data.if_statement.else_body, global, &if_scope);

            sym_table_cleanup(&if_scope);
            break;
        }

        case AST_RETURN: {
            _analyze_expression(global, variables, node->data.expr);
            break;
        }

        case AST_UNARY_OP:
        case AST_BINARY_OP: {
            _analyze_expression(global, variables, node);
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
            const char* FnName = node->data.function_declaration.name;
            // Multiple definitions?
            {
                ast_node_t* fn = sym_table_get(&global->functions, FnName);
                if (fn) {
                    ANALYZER_ERROR(node->position, "Function '%s' is defined more than once!", FnName);
                }
            }

            // Return type valid
            const datatype_t* VarType = &node->data.function_declaration.return_type;
            const bool IsTypeValid = _analyze_is_valid_type(global, VarType);
            if (!IsTypeValid) {
                const datatype_t* UnderlyingType = datatype_underlying_type(VarType);
                ANALYZER_ERROR(node->position, "In function '%s' return type '%s' is not defined!",  FnName, UnderlyingType->data.typename);
            }
            sym_table_insert(&global->functions, node->data.function_declaration.name, node);

            // Main specific
            if (strcmp(FnName, "main") == 0) {
                if (VarType->kind != DATATYPE_CORE_TYPE || VarType->data.builtin != CORETYPE_I32) {
                    ANALYZER_ERROR(node->position, "The main function can only return 'i32'");
                }
            }

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

