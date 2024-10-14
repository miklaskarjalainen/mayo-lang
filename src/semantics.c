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

static const datatype_t* _analyze_expression(const global_scope_t* global, const sym_table_t* variables, ast_node_t* expr);

static bool _analyze_is_valid_type(const global_scope_t* global, const datatype_t* type) {
    const datatype_t* TrueType = datatype_underlying_type(type);
    DEBUG_ASSERT(TrueType->kind == DATATYPE_PRIMITIVE, "?");

    if (strcmp(TrueType->typename, "i32") == 0) {
        return true;
    }
    else if (strcmp(TrueType->typename, "bool") == 0) {
        return true;
    }

    ast_variable_declaration_t* var_decl = sym_table_get(&global->structs, TrueType->typename);
    return var_decl != NULL;
}

static void _analyze_func_call(const global_scope_t* global, const sym_table_t* variables, ast_node_t* node) {
    DEBUG_ASSERT(node->kind == AST_FUNCTION_CALL, "?");
    const ast_function_call_t* FuncCall = &node->data.function_call;
            
    // Func exists?
    const ast_node_t* FuncDecl = sym_table_get(&global->functions, FuncCall->name);
    if (!FuncDecl){
        ANALYZER_ERROR(node->position, "No function called '%s' is exists!", FuncCall->name);
    }

    // Check that the args match function declaration.
    const size_t CallArgCount = arrlenu(FuncCall->args);
    const size_t DeclArgCount = arrlenu(FuncDecl->data.function_declaration.args);
    
    if (CallArgCount != DeclArgCount) {
        ANALYZER_ERROR(node->position, "function takes %zu arguments but %zu were given!", DeclArgCount, CallArgCount);
    }

    for (size_t i = 0; i < CallArgCount; i++) {
        ast_variable_declaration_t* argument_decl = &FuncDecl->data.function_declaration.args[i].data.variable_declaration;
        ast_node_t* argument_expr = FuncCall->args[i];
        
        const datatype_t* ExprType = _analyze_expression(global, variables, argument_expr);
        if (!datatype_cmp(ExprType, &argument_decl->type)) {
            ANALYZER_ERROR(node->position, "passed argument does not match expected type for argument!");
        }
    }
}

static const datatype_t* _analyze_expression(const global_scope_t* global, const sym_table_t* variables, ast_node_t* expr) {
    static const datatype_t BoolType = {
        .kind = DATATYPE_PRIMITIVE,
        .typename = "bool"
    };
    static const datatype_t I32Type = {
        .kind = DATATYPE_PRIMITIVE,
        .typename = "i32"
    };

    switch (expr->kind) {
        case AST_INTEGER_LITERAL: {
            return &I32Type;
        };

        case AST_GET_VARIABLE: {
            ast_node_t* var_decl = sym_table_get(variables, expr->data.literal);
            if (!var_decl) {
                ANALYZER_ERROR(expr->position, "No variable called '%s' exists, used in expression!", expr->data.literal);
            }

            return &var_decl->data.variable_declaration.type;
        }

        case AST_FUNCTION_CALL: {
            _analyze_func_call(global, variables, expr);
            ast_node_t* func_decl = sym_table_get(&global->functions, expr->data.literal);
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

        case AST_ARRAY_INITIALIZER_LIST: {
            UNIMPLEMENTED("No implemented yet, requires a rework of how datatypes work.");
            break;
        }

        default: {
            PANIC("not implemented for type %u", (uint32_t)expr->kind);
        }
    }
    
    return NULL;
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
                ast_node_t* var_decl = sym_table_get(variables, VarName);
                if (var_decl){
                    ANALYZER_ERROR(node->position, "Variable called '%s' is already defined!", VarName);
                }
            }

            // Has a valid type?
            const datatype_t* VarType = &node->data.variable_declaration.type;
            const bool IsTypeValid = _analyze_is_valid_type(global, VarType);
            if (!IsTypeValid) {
                const datatype_t* UnderlyingType = datatype_underlying_type(VarType);
                ANALYZER_ERROR(node->position, "Type '%s' is not defined!", UnderlyingType->typename);
            }

            // Get type of expression
            if (node->data.variable_declaration.expr) {
                const datatype_t* ExprType = _analyze_expression(global, variables, node->data.variable_declaration.expr);
                
                if (!datatype_cmp(VarType, ExprType)) {
                    ANALYZER_ERROR(node->position, "Type mismatch between declaration and expression!");
                }
            }

            sym_table_insert(variables, VarName, node);
            break;
        }

        case AST_IF_STATEMENT: {
            // expr
            _analyze_expression(global, variables, node->data.if_statement.expr);

            sym_table_t if_scope = { 0 };
            sym_table_init(&if_scope);
            if_scope.parent = variables;

            CALL_ON_BODY(_analyze_scoped_node, node->data.if_statement.body, global, &if_scope);
            sym_table_clear(&if_scope);
            CALL_ON_BODY(_analyze_scoped_node, node->data.if_statement.else_body, global, &if_scope);

            sym_table_cleanup(&if_scope);
            break;
        }

        case AST_FUNCTION_CALL: {
            _analyze_func_call(global, variables, node);
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
                ANALYZER_ERROR(node->position, "In function '%s' return type '%s' is not defined!",  FnName, UnderlyingType->typename);
            }

            // Main specific
            if (strcmp(FnName, "main") == 0) {
                if (VarType->kind != DATATYPE_PRIMITIVE || strcmp(VarType->typename, "i32")) {
                    ANALYZER_ERROR(node->position, "The main function can only return 'i32'");
                }
            }
            sym_table_insert(&global->functions, node->data.function_declaration.name, node);
            
            // Check parameters & Arguments to the function's scope
            sym_table_t fn_scope;
            sym_table_init(&fn_scope);
            const size_t Size = arrlenu(node->data.function_declaration.args);
            for (size_t arg = 0; arg < Size; arg++) {
                ast_node_t* argument = &node->data.function_declaration.args[arg]; 
                _analyze_scoped_node(argument, global, &fn_scope);
                sym_table_insert(&fn_scope, argument->data.variable_declaration.name, (void*)argument);
            }

            // Analyze body
            CALL_ON_BODY(_analyze_scoped_node, node->data.function_declaration.body, global, &fn_scope);
            sym_table_cleanup(&fn_scope);
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

