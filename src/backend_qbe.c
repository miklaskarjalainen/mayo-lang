#include <stdio.h>
#include <stdint.h>
#include <stb/stb_ds.h>

#include "common/error.h"
#include "common/utils.h"
#include "parser/ast_type.h"

#include "backend_qbe.h"

typedef struct temporary_t {
    uint32_t id;
} temporary_t;

typedef struct label_t {
    uint32_t id;
} label_t;

typedef struct qbe_variable_t {
    const char* var_name;
    temporary_t temp;
} qbe_variable_t;

static temporary_t get_temporary(void) {
    static uint32_t s_Id = 0;
    return (temporary_t){.id = s_Id++};
}

static void fprint_temp(FILE* f, temporary_t temp) {
    fprintf(f, "%%r%u", temp.id);
} 

/*
static label_t get_label(void) {
    static uint32_t s_Id = 0;
    return (label_t){.id = s_Id++};
}
*/

static temporary_t _generate_expr_node(FILE* f, ast_node_t* ast, qbe_variable_t* variables) {
    switch (ast->kind) {
        case AST_INTEGER_LITERAL: {
            temporary_t r = get_temporary();
            fprintf(f, "\t");
            fprint_temp(f, r);
            fprintf(f, "=w copy %li\n", ast->data.integer);
            return r;
        }

        case AST_GET_VARIABLE: {
            // Search for temp
            size_t VarLen = arrlenu(variables);
            for (size_t i = 0; i < VarLen; i++) {
                if (strcmp(variables[i].var_name, ast->data.literal) == 0) {
                    return variables[i].temp;
                }
            }
            PANIC("no temporary for variable '%s'!", ast->data.literal);
            break;
        }

        case AST_BINARY_OP: {
            temporary_t lhs = _generate_expr_node(f, ast->data.binary_op.left, variables);
            temporary_t rhs = _generate_expr_node(f, ast->data.binary_op.right, variables);
            temporary_t r = get_temporary();
            
            fprintf(f, "\t");
            fprint_temp(f, r);
            
            switch (ast->data.binary_op.operation) {
                case BINARY_OP_ADD: { fprintf(f, "=w add "); break; }
                case BINARY_OP_MULTIPLY: { fprintf(f, "=w mul "); break; }

                default: {
                    PANIC("Op not implemented %u", ast->data.binary_op.operation);
                }
            }

            fprint_temp(f, lhs);
            fprintf(f, ", ");
            fprint_temp(f, rhs);
            fprintf(f, "\n");
            return r;
        }

        case AST_FUNCTION_CALL: {
            const ast_function_call_t* FuncCall = &ast->data.function_call;

            // Make temporaries for the arguments
            temporary_t* arg_temps = NULL;
            const size_t ArgCount = arrlenu(FuncCall->args);
            for (size_t i = 0; i < ArgCount; i++) {
                arrput(arg_temps, _generate_expr_node(f, FuncCall->args[i], variables));
            }

            // 
            temporary_t r = get_temporary();
            fprintf(f, "\t");
            fprint_temp(f, r);
            fprintf(f, "=w call $%s(", FuncCall->name);

            // Pass arguments to the function call
            for (size_t i = 0; i < ArgCount; i++) {
                fprintf(f, "w ");
                fprint_temp(f, arg_temps[i]);
                fprintf(f, ", ");
            }
            fprintf(f, ")\n");
            arrfree(arg_temps);
            return r;
        }

        case AST_RETURN: {
            temporary_t r = _generate_expr_node(f, ast->data.expr, variables);
            fprintf(f, "\tret ");
            fprint_temp(f, r);
            fprintf(f, "\n");
            return r;
        }

        case AST_VARIABLE_DECLARATION: {
            const char* ArgName = ast->data.variable_declaration.name;
            temporary_t r = _generate_expr_node(f, ast->data.variable_declaration.expr, variables);
            const qbe_variable_t VarTemp = {.var_name = ArgName, .temp = r};
            arrput(variables, VarTemp);
            break;
        }

        default: {
            PANIC("not implemented for type %u", ast->kind);
        }
    }

    return get_temporary();
}

static void _generate_ast_global_node(FILE* f, ast_node_t* ast) {

    switch (ast->kind) {
        case AST_FUNCTION_DECLARATION: {
            const ast_function_declaration_t* FuncDecl = &ast->data.function_declaration;
            
            qbe_variable_t* variables = NULL; //? Pointer to this might need to passed (double ptr)

            if (strcmp(FuncDecl->name, "main") == 0) {
                fprintf(f, "export ");
            }

            fprintf(f, "function w $%s(", FuncDecl->name);
            const size_t ArrCount = arrlenu(FuncDecl->args);
            for (size_t i = 0; i < ArrCount; i++) {
                const char* ArgName = FuncDecl->args[i].data.variable_declaration.name;
                const qbe_variable_t VarTemp = {.var_name = ArgName, .temp = get_temporary()};
                arrput(variables, VarTemp);

                fprintf(f, "w ");
                fprint_temp(f, VarTemp.temp);
                fprintf(f, ", ");
            }
            fprintf(f, ") {\n@start\n");
            
            // Body
            const size_t BodyCount = arrlenu(FuncDecl->body);
            for (size_t i = 0; i < BodyCount; i++) {
                _generate_expr_node(f, FuncDecl->body[i], variables);
            }
            arrfree(variables);

            fprintf(f, "}\n");
            break;
        }

        default: {
            PANIC("not implemented for type %u", ast->kind);
        }
    }

}

void generate_qbe(FILE* f, ast_node_t* ast) {
    DEBUG_ASSERT(ast->kind == AST_TRANSLATION_UNIT, "?");
    
    const size_t Len = arrlenu(ast->data.translation_unit.body);
    for (size_t i = 0; i < Len; i++) {
        _generate_ast_global_node(f, ast->data.translation_unit.body[i]);
    }
}
