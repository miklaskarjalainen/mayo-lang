#include <stb/stb_ds.h>
#include <string.h>

#include "../parser/ast_type.h"
#include "../common/error.h"
#include "../common/utils.h"
#include "typechecker.h"

#define MAX_STACKS_SIZE 64

typedef struct ir_gen_data {
    ast_function_declaration_t* function_decls[MAX_STACKS_SIZE];
    ast_variable_declaration_t* variable_decls[MAX_STACKS_SIZE];
    ast_struct_declaration_t* struct_decls[MAX_STACKS_SIZE];
    size_t stack_depth;
} ir_gen_data;

static void ir_push_stack(ir_gen_data* data) {
    RUNTIME_ASSERT(data->stack_depth < MAX_STACKS_SIZE, "stack overflow");
    data->stack_depth++;    
}

static void ir_pop_stack(ir_gen_data* data) {
    RUNTIME_ASSERT(data->stack_depth > 0, "stack underflow");
    arrfree(data->function_decls[data->stack_depth]);
    arrfree(data->variable_decls[data->stack_depth]);
    arrfree(data->struct_decls[data->stack_depth]);
    data->stack_depth--;
}

static ast_function_declaration_t* ir_get_function(const char* name, ir_gen_data* data) {
    for (size_t i = 0; i <= data->stack_depth; i++) {
        for (size_t j = 0; j < arrlenu(data->function_decls[i]); j++) {
            if (data->function_decls[i] == NULL) { continue; }
            if (strcmp(data->function_decls[i][j].name, name) == 0) {
                return &data->function_decls[i][j];
            }
        }
    }
    return NULL;
}

static void ir_declare_function(ast_function_declaration_t decl, ir_gen_data* data) {
    RUNTIME_ASSERT(ir_get_function(decl.name, data) == NULL, "function with a name '%s' is already defined!", decl.name);

    INFO("declared fn '%s'", decl.name);
    arrpush(data->function_decls[data->stack_depth], decl);
}

static void ir_declare_struct(ast_struct_declaration_t decl, ir_gen_data* data) {
    INFO("declared struct '%s'", decl.name);
    arrpush(data->struct_decls[data->stack_depth], decl);
}

/*
static void ir_declare_variable(ir_gen_data* data, ast_variable_declaration_t decl) {
    arrpush(data->variable_decls[data->stack_depth], decl);
}

static ast_struct_declaration_t* ir_get_struct(ir_gen_data* data, const char* name) {
    for (size_t i = 0; i < data->stack_depth; i++) {
        for (size_t j = 0; j < arrlenu(data->struct_decls[i]); j++) {
            if (strcmp(data->struct_decls[i][j].name, name) == 0) {
                return &data->struct_decls[i][j];
            }
        }
    }
    RUNTIME_ASSERT(false, "struct '%s' not found", name);
}

static ast_variable_declaration_t* ir_get_variable(ir_gen_data* data, const char* name) {
    for (size_t i = 0; i < data->stack_depth; i++) {
        for (size_t j = 0; j < arrlenu(data->struct_decls[i]); j++) {
            if (strcmp(data->variable_decls[i][j].name, name) == 0) {
                return &data->variable_decls[i][j];
            }
        }
    }
    RUNTIME_ASSERT(false, "variable '%s' not found", name);
}
*/

#define CALL_ON_BODY(fn, body, ...)             \
    do {                                        \
        size_t BodyLen = arrlenu(body);         \
        for (size_t i = 0; i < BodyLen; i++) {  \
            fn(body[i], __VA_ARGS__);           \
        }                                       \
    } while(0)

static ir_instruction_t* ast_to_ir_internal(ast_node_t* ast, ir_gen_data* data) {
    RUNTIME_ASSERT(ast != NULL, "ast is null");

    UNUSED(ast);
    UNUSED(data);

    switch (ast->kind) {
    case AST_STRUCT_DECLARATION: {
        ir_declare_struct(ast->data.struct_declaration, data);
        break;
    }
    case AST_FUNCTION_DECLARATION: {
        ir_declare_function(ast->data.function_declaration, data);
        ir_push_stack(data);
        CALL_ON_BODY(ast_to_ir_internal, ast->data.function_declaration.body, data);
        ir_pop_stack(data);
        break;
    }
    case AST_TRANSLATION_UNIT: {
        ir_push_stack(data);
        CALL_ON_BODY(ast_to_ir_internal, ast->data.translation_unit.body, data);
        ir_pop_stack(data);
        break;
    }
    case AST_FOR_LOOP: {
        // TODO: expr
        ir_push_stack(data);
        CALL_ON_BODY(ast_to_ir_internal, ast->data.for_loop.body, data);
        ir_pop_stack(data);
        break;
    }
    case AST_WHILE_LOOP: {
        // TODO: expr
        ir_push_stack(data);
        CALL_ON_BODY(ast_to_ir_internal, ast->data.while_loop.body, data);
        ir_pop_stack(data);
        break;
    }
    case AST_IF_STATEMENT: {
        // TODO: expr
        ir_push_stack(data);
        CALL_ON_BODY(ast_to_ir_internal, ast->data.if_statement.body, data);
        ir_pop_stack(data);
        ir_push_stack(data);
        CALL_ON_BODY(ast_to_ir_internal, ast->data.if_statement.else_body, data);
        ir_pop_stack(data);
        break;
    }
    case AST_FUNCTION_CALL: {
        const char* FnName = ast->data.function_call.name;
        ast_function_declaration_t* fn = ir_get_function(FnName, data);
        RUNTIME_ASSERT(fn, "no function named '%s'", FnName);
        break;
    }
    
    case AST_RETURN: {
        if (ast->data.expr) {
            ast_to_ir_internal(ast->data.expr, data);
        }
        break;
    }

    default:
        break;
    }

    return NULL;
}

ir_instruction_t* ast_to_ir(ast_node_t* ast) {
    ir_gen_data data = { 0 };
    return ast_to_ir_internal(ast, &data);
}
