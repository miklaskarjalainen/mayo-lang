#include <stb/stb_ds.h>
#include <stdio.h>

#include "semantics.h"
#include "parser.h"
#include "string.h"
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
    
    // Work around for having to allocate datatypes for inner types.
    arena_t* arena;
} global_scope_t;

static datatype_t _analyze_expression(global_scope_t* global, const sym_table_t* variables, ast_node_t* expr);

static bool _analyze_is_valid_type(const global_scope_t* global, const datatype_t* type) {
    const datatype_t* TrueType = datatype_underlying_type(type);
    if (type->kind == DATATYPE_VARIADIC) {
        return true;
    }
    DEBUG_ASSERT(TrueType->kind == DATATYPE_PRIMITIVE, "?");

    if (strcmp(TrueType->typename, "void") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "u8") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "u16") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "i32") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "i64") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "bool") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "char") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "f32") == 0) {
        return true;
    }
    if (strcmp(TrueType->typename, "f64") == 0) {
        return true;
    }

    ast_variable_declaration_t* var_decl = sym_table_get(&global->structs, TrueType->typename);
    return var_decl != NULL;
}

static void _analyze_func_call(global_scope_t* global, const sym_table_t* variables, ast_node_t* node) {
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
    const bool IsVariadic = DeclArgCount ? 
        FuncDecl->data.function_declaration.args[DeclArgCount-1].data.variable_declaration.type.kind == DATATYPE_VARIADIC : false;
    
    if (!IsVariadic && (CallArgCount != DeclArgCount)) {
        ANALYZER_ERROR(node->position, "function takes %zu arguments but %zu were given!", DeclArgCount, CallArgCount);
    }
    if (IsVariadic && (CallArgCount < DeclArgCount-1)) {
        ANALYZER_ERROR(node->position, "variadic function takes atleast %zu arguments but %zu were given!", DeclArgCount-1, CallArgCount);
    }

    for (size_t i = 0; i < CallArgCount; i++) {
        // Arguments for the variadic part are not type-checked.
        const bool CheckArgType = IsVariadic && (DeclArgCount - 1 > i);

        ast_variable_declaration_t* argument_decl = &FuncDecl->data.function_declaration.args[i].data.variable_declaration;
        ast_node_t* argument_expr = FuncCall->args[i];
        
        datatype_t ExprType = _analyze_expression(global, variables, argument_expr);
        if (CheckArgType && !datatype_cmp(&argument_decl->type, &ExprType)) {
            char expr_type_str[0xFF] = { 0 };
            strncpy(expr_type_str, datatype_to_str(&ExprType), ARRAY_LEN(expr_type_str));
            ANALYZER_ERROR(node->position, "Argument expected type '%s', got '%s' instead!", datatype_to_str(&argument_decl->type), expr_type_str);
        }
        argument_expr->expr_type = ExprType;
    }

    // @HACK: Insert a ghost variable decl to call arguments for the backend so it knows when the variadic parameters begin.
    if (IsVariadic) {
        static ast_node_t ghost_var = {
            .kind = AST_VARIABLE_DECLARATION,
            .data.variable_declaration = {
                .type = {
                    .kind = DATATYPE_VARIADIC
                }
            }
        };
        arrins(node->data.function_call.args, DeclArgCount-1, &ghost_var);
    }

    // @HACK: also set in _analyze_expression, but _analyze_func_call can also be called somewhere else.
    node->expr_type = FuncDecl->data.function_declaration.return_type;
}

static datatype_t _analyze_expression_impl(global_scope_t* global, const sym_table_t* variables, ast_node_t* expr) {
    static const datatype_t BoolType = {
        .kind = DATATYPE_PRIMITIVE,
        .typename = "bool"
    };
    static const datatype_t CharType = {
        .kind = DATATYPE_PRIMITIVE,
        .typename = "char"
    };

    switch (expr->kind) {
        case AST_BOOL_LITERAL: { return BoolType; };
        case AST_CHAR_LITERAL: { return CharType; };

        case AST_FLOAT_LITERAL: {
            return (datatype_t) {
                .kind = DATATYPE_PRIMITIVE,
                .typename = "f32"
            };
        };

        case AST_INTEGER_LITERAL: {
            return (datatype_t) {
                .kind = DATATYPE_PRIMITIVE,
                .typename = "i32"
            };
        };

        case AST_STRING_LITERAL: {
            return (datatype_t) {
                .kind = DATATYPE_ARRAY,
                .base = &CharType,
                .array_size = strlen(expr->data.literal)+1
            };
        };

        case AST_GET_VARIABLE: {
            ast_node_t* var_decl = sym_table_get(variables, expr->data.literal);
            if (!var_decl) {
                ANALYZER_ERROR(expr->position, "No variable called '%s' exists, used in expression!", expr->data.literal);
            }

            return var_decl->data.variable_declaration.type;
        }

        case AST_FUNCTION_CALL: {
            _analyze_func_call(global, variables, expr);
            ast_node_t* func_decl = sym_table_get(&global->functions, expr->data.literal);
            DEBUG_ASSERT(func_decl->kind == AST_FUNCTION_DECLARATION, "?");
            return func_decl->data.function_declaration.return_type;
        }

        case AST_UNARY_OP: {
            // Create a type which a pointer to this one.
            datatype_t* inner = arena_alloc(global->arena, sizeof(datatype_t));
            *inner = _analyze_expression(global, variables, expr->data.unary_op.operand);

            return (datatype_t) {
                .kind = DATATYPE_POINTER,
                .base = inner
            };
        }

        case AST_BINARY_OP: {
            const datatype_t Lhs = _analyze_expression(global, variables, expr->data.binary_op.left);
            const datatype_t Rhs = _analyze_expression(global, variables, expr->data.binary_op.right);
            if (expr->data.binary_op.operation == BINARY_OP_ARRAY_INDEX) {
                if (Lhs.kind != DATATYPE_ARRAY) {
                    ANALYZER_ERROR(expr->position, "Array index: array expected!");
                }
                return *Lhs.base;
            }

            if (!datatype_cmp(&Lhs, &Rhs)) {
                ANALYZER_ERROR(expr->position, "Type mismatch!");
            }

            switch (expr->data.binary_op.operation) {
                case BINARY_OP_LESS_THAN:
                case BINARY_OP_LESS_OR_EQUAL_THAN:
                case BINARY_OP_GREATER_THAN:
                case BINARY_OP_GREATER_OR_EQUAL_THAN:
                case BINARY_OP_NOT_EQUAL:
                case BINARY_OP_EQUAL: {
                    return BoolType;
                }

                default: {
                    return Lhs;
                }
            }
        }

        case AST_STRUCT_INITIALIZER_LIST: {
            const ast_struct_initializer_list_t* Initializer = &expr->data.struct_initializer_list; 
            
            // Basic type checking.
            const datatype_t Type = {
                .kind = DATATYPE_PRIMITIVE,
                .typename = Initializer->name,
            };
            if (!_analyze_is_valid_type(global, &Type)) {
                ANALYZER_ERROR(expr->position, "Invalid type name for struct initializer");
            }
            const ast_node_t* StructDecl = sym_table_get(&global->structs, Initializer->name);
            DEBUG_ASSERT(StructDecl->kind == AST_STRUCT_DECLARATION, "?");

            // Check that the initializer initializes actual members and do typechecking on the expressions.
            const size_t InitializerCount = arrlenu(Initializer->fields);
            const size_t StructMemberCount = arrlenu(StructDecl->data.struct_declaration.members);
            for (size_t i = 0; i < InitializerCount; i++) {
                const char* InitializerField = Initializer->fields[i].name;

                for (size_t j = 0; j < StructMemberCount; j++) {
                    // Does this initializer match a member?
                    const char* StructMember = StructDecl->data.struct_declaration.members[i].name;
                    if (strcmp(InitializerField, StructMember)) {
                        if (j == StructMemberCount-1) {
                            ANALYZER_ERROR(expr->position, "Struct does not contain a field called '%s'", InitializerField);
                        }
                        continue;
                    }

                    // Matching field
                    const datatype_t ExprType = _analyze_expression(global, variables, Initializer->fields[i].expr);
                    const datatype_t* MemberType = &StructDecl->data.struct_declaration.members[i].type;

                    if (!datatype_cmp(MemberType, &ExprType)) {
                        ANALYZER_ERROR(Initializer->fields[i].expr->position, "expression not matching type! expected %s!", datatype_to_str(MemberType));
                    }

                    break;
                }
            }

            if (InitializerCount != StructMemberCount) {
                ANALYZER_ERROR(expr->position, "Initializer list only initializes %zu of the %zu members!", InitializerCount, StructMemberCount);
            }

            return Type;
        }

        case AST_ARRAY_INITIALIZER_LIST: {
            ast_node_t** initializer_list = expr->data.array_initializer_list.exprs;
            const size_t InitializerSize = arrlenu(initializer_list);
            if (InitializerSize == 0) {
                ANALYZER_ERROR(expr->position, "cannot deduce type from an array which is 0 size");
            }

            // Get the type of the first initializer.
            const datatype_t FirstExprType = _analyze_expression(global, variables, initializer_list[0]);

            // And check that every other initializer matches the type of the first.
            for (size_t i = 1; i < InitializerSize; i++) {
                const datatype_t ExprType = _analyze_expression(global, variables, initializer_list[i]);
                if (!datatype_cmp(&FirstExprType, &ExprType)) {
                    ANALYZER_ERROR(initializer_list[i]->position, "Invalid types used in initializer list!");
                }
            }

            // construct type for this
            datatype_t* inner_type = arena_alloc(global->arena, sizeof(datatype_t));
            *inner_type = FirstExprType;             
            return (datatype_t) {
                .kind = DATATYPE_ARRAY,
                .base = inner_type,
                .array_size = InitializerSize
            };
        }

        case AST_CAST_STATEMENT: {
            const datatype_t TargetType = expr->data.cast_statement.target_type;
            ast_node_t* cast_expr = expr->data.cast_statement.expr;
            
            // Contains expr
            if (!cast_expr) {
                ANALYZER_ERROR(expr->position, "Empty cast statement");
            }

            // Target type is valid
            if (!_analyze_is_valid_type(global, &TargetType)) {
                ANALYZER_ERROR(expr->position, "Target type '%s' is not defined!", datatype_to_str(&TargetType));
            }

            // Is castable.
            const datatype_t ExprType = _analyze_expression(global, variables, cast_expr);
            if (datatype_cmp(&TargetType, &ExprType)) {
                return TargetType;
            }

            // @FIXME: hard coded.
            if (TargetType.kind == DATATYPE_POINTER && ExprType.kind == DATATYPE_POINTER) {
                return TargetType;
            }
            if (TargetType.kind == DATATYPE_PRIMITIVE && ExprType.kind == DATATYPE_PRIMITIVE) {
                // i32 <-> bool
                if (
                    (!strcmp(TargetType.typename, "bool") && !strcmp(ExprType.typename, "i32")) ||
                    (!strcmp(TargetType.typename, "i32") && !strcmp(ExprType.typename, "bool"))
                    ) {
                    return TargetType;
                }
                // i32 <-> char
                if (
                    (!strcmp(TargetType.typename, "char") && !strcmp(ExprType.typename, "i32")) ||
                    (!strcmp(TargetType.typename, "i32") && !strcmp(ExprType.typename, "char"))
                    ) {
                    return TargetType;
                }
                // i32 <-> i64
                if (
                    (!strcmp(TargetType.typename, "i64") && !strcmp(ExprType.typename, "i32")) ||
                    (!strcmp(TargetType.typename, "i32") && !strcmp(ExprType.typename, "i64"))
                    ) {
                    return TargetType;
                }
                // i32 <-> u8
                if (
                    (!strcmp(TargetType.typename, "u8") && !strcmp(ExprType.typename, "i32")) ||
                    (!strcmp(TargetType.typename, "i32") && !strcmp(ExprType.typename, "u8"))
                    ) {
                    return TargetType;
                }
            }

            ANALYZER_ERROR(expr->position, "Expression cannot be cast to type %s", datatype_to_str(&TargetType));
            break;
        }

        case AST_GET_MEMBER: {
            const ast_get_member_t* GetMember = &expr->data.get_member;
            const datatype_t ExprType = _analyze_expression(global, variables, GetMember->expr);
            const char* GetMemberName = GetMember->member;
            if (ExprType.kind != DATATYPE_PRIMITIVE) {
                ANALYZER_ERROR(expr->position, "Structure expected!");
            }
            const ast_node_t* Decl = sym_table_get(&global->structs, ExprType.typename);
            if (!Decl){
                if (_analyze_is_valid_type(global, &ExprType)) {
                    ANALYZER_ERROR(expr->position, "Expression has no member called '%s'!", GetMemberName);
                }
                ANALYZER_ERROR(expr->position, "No typename '%s' is defined!", ExprType.typename);
            }

            // Find field type
            DEBUG_ASSERT(Decl->kind == AST_STRUCT_DECLARATION, "?");
            const size_t MemberCount = arrlenu(Decl->data.struct_declaration.members);
            for (size_t i = 0; i < MemberCount; i++) {
                const ast_variable_declaration_t* MemberDecl = &Decl->data.struct_declaration.members[i];
                if (strcmp(MemberDecl->name, GetMemberName) == 0) {
                    return MemberDecl->type;
                }
            }

            ANALYZER_ERROR(expr->position, "Expression has no member called '%s'!", GetMemberName);
            return (datatype_t){ 0 };
        }

        default: {
            ANALYZER_ERROR(expr->position, "Semantics not implemented for type %u", (uint32_t)expr->kind);
        }
    }

    return (datatype_t){ 0 };
}

static datatype_t _analyze_expression(global_scope_t* global, const sym_table_t* variables, ast_node_t* expr) {
    expr->expr_type = _analyze_expression_impl(global, variables, expr);
    return expr->expr_type;
}

static void _analyze_scoped_node(ast_node_t* node, global_scope_t* global, sym_table_t* variables) {
    if (!node) {
        return;
    }

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
                datatype_t ExprType = _analyze_expression(global, variables, node->data.variable_declaration.expr);
                
                if (!datatype_cmp(VarType, &ExprType)) {
                    char expr_type_str[0xFF] = { 0 };
                    strncpy(expr_type_str, datatype_to_str(&ExprType), ARRAY_LEN(expr_type_str));
                    ANALYZER_ERROR(node->data.variable_declaration.expr->position, "Expression expected type '%s' got '%s'!", datatype_to_str(VarType), expr_type_str);
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

        case AST_WHILE_LOOP: {
            // expr
            _analyze_expression(global, variables, node->data.while_loop.expr);

            sym_table_t loop_scope = { 0 };
            sym_table_init(&loop_scope);
            loop_scope.parent = variables;
            CALL_ON_BODY(_analyze_scoped_node, node->data.while_loop.body, global, &loop_scope);
            sym_table_cleanup(&loop_scope);
            break;
        }

        case AST_FUNCTION_CALL: {
            _analyze_func_call(global, variables, node);
            break;
        }

        case AST_RETURN: {
            if (node->data.expr) {
                _analyze_expression(global, variables, node->data.expr);
            }
            break;
        }

        case AST_UNARY_OP:
        case AST_BINARY_OP: {
            _analyze_expression(global, variables, node);
            break;
        }

        default: {
            ANALYZER_ERROR(node->position, "Unhandled!");
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
            
            // Check parameters & add them to the function's scope
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
            PANIC("Unhandled node!");
            break;
        }
    }
    
}

void semantic_analysis(arena_t* arena, ast_node_t* node) {
    global_scope_t global = { 0 };
    sym_table_init(&global.functions);
    sym_table_init(&global.structs);
    global.arena = arena;

    _analyze_global_node(node, &global);

    sym_table_cleanup(&global.functions);
    sym_table_cleanup(&global.structs);
}

