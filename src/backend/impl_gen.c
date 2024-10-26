#include <stdio.h>
#include <stdint.h>
#include <stb/stb_ds.h>

#include "../common/error.h"
#include "../common/utils.h"
#include "../parser/ast_type.h"
#include "../backend_qbe.h"
#include "impl_gen.h"

void qbe_generate_struct_members(FILE* f, const ast_struct_declaration_t* decl, backend_ctx_t* ctx) {
    const size_t MemCount = arrlenu(decl->members);
    for (size_t i = 0; i < MemCount; i++) {
        char c = qbe_get_base_type(&decl->members[i].type);
        if (c != '\0') {
            fprintf(f, "%c, ", qbe_get_base_type(&decl->members[i].type));
        }
        else {
            const datatype_t MemberType = decl->members[i].type; 
            RUNTIME_ASSERT(MemberType.kind == DATATYPE_PRIMITIVE, "only primitive types are supported!");
            const aggregate_type_t* InnerStruct = qbe_find_type(MemberType.typename, ctx);
            qbe_generate_struct_members(f, InnerStruct->ast, ctx);
        }
    }
}

temporary_t qbe_generate_string_literal(
    FILE* f,
    const ast_node_t* ast
) {
    DEBUG_ASSERT(ast && ast->kind == AST_STRING_LITERAL, "?");

    const size_t AllocSize = strlen(ast->data.literal) + 1;

    // Allocate storage for the string
    const temporary_t ArrayBegin  = get_temporary();
    fprintf(f, "\t");
    fprint_temp(f, ArrayBegin);
    fprintf(f, "=l alloc4 %zu\n", AllocSize);

    // Store the characters from the string to the array
    for (size_t i = 0; i < AllocSize; i++) {
        const temporary_t IndexPtr  = get_temporary();
        const char Char = ast->data.literal[i];

        // Ptr to index
        fprintf(f, "\t");
        fprint_temp(f, IndexPtr);
        fprintf(f, " =l add ");
        fprint_temp(f, ArrayBegin);
        fprintf(f, ", %zu # Str[%zu] \n", i, i);

        // Store a byte into index
        fprintf(f, "\tstoreb %u, ", Char);
        fprint_temp(f, IndexPtr);
        fprintf(f, " # Str[%zu] <- %u\n", i, Char);
    }

    return ArrayBegin;
}

temporary_t qbe_generate_array_initializer(
    FILE* f,
    const ast_node_t* ast,
    backend_ctx_t* ctx
) {
    DEBUG_ASSERT(ast && ast->kind == AST_ARRAY_INITIALIZER_LIST, "?");

    const datatype_t* ExprType = &ast->expr_type;
    const size_t ElementSize = qbe_get_type_size(ExprType->base, ctx);
    const size_t ArraySize = ExprType->array_size;
    const size_t AllocSize = ArraySize * ElementSize;

    // Allocate storage for the string
    const temporary_t ArrayBegin  = get_temporary();
    fprintf(f, "\t");
    fprint_temp(f, ArrayBegin);
    fprintf(f, "=l alloc4 %zu\n", AllocSize);

    // Store the characters from the string to the array
    for (size_t i = 0; i < ArraySize; i++) {
        // Create temporary for the index
        const temporary_t Index = get_temporary();
        fprintf(f, "\t");
        fprint_temp(f, Index);
        fprintf(f, "=w copy %zu\n", i);
        const temporary_t IndexPtr  = qbe_get_array_ptr(f, ArrayBegin, Index, ExprType->base, ctx);

        // Get expr
        fprintf(f, "# Array[%zu] expr \n", i);
        const temporary_t ValueTemp  = qbe_generate_expr_node(f, ast->data.array_initializer_list.exprs[i], ctx);

        // Store a byte into index
        fprintf(f, "\t%s ", qbe_get_store_ins(ExprType->base));
        fprint_temp(f, ValueTemp);
        fprintf(f, ", ");
        fprint_temp(f, IndexPtr);
        fprintf(f, "\n");
    }

    return ArrayBegin;
}

temporary_t qbe_generate_function_call(
    FILE* f,
    const ast_node_t* ast,
    backend_ctx_t* ctx
) {
    DEBUG_ASSERT(ast && ast->kind == AST_FUNCTION_CALL, "?");
    const ast_function_call_t* FuncCall = &ast->data.function_call;


    // Make temporaries for the arguments
    const size_t ArgCount = arrlenu(FuncCall->args);
    temporary_t* arg_temps = NULL;
    {
        bool variadic_arguments = false;
        for (size_t i = 0; i < ArgCount; i++) {
            if (FuncCall->args[i]->data.variable_declaration.type.kind == DATATYPE_VARIADIC) {
                variadic_arguments = true;
                continue;
            }

            temporary_t expr_temp = qbe_generate_expr_node(f, FuncCall->args[i], ctx);
            
            // argument promotion
            // TODO: Implement ints. (only floats to doubles atm)
            if (variadic_arguments) {
                if (strcmp(FuncCall->args[i]->expr_type.typename, "f32") == 0) {
                    temporary_t r = get_temporary();
                    fprintf(f, "\t");
                    fprint_temp(f, r);
                    fprintf(f, "=d exts ");
                    fprint_temp(f, expr_temp);
                    fprintf(f, "\n");
                    expr_temp = r;
                }
            }

            arrput(arg_temps, expr_temp);
        }
    }

    // 
    temporary_t r = get_temporary();
    fprintf(f, "\t");
    fprint_temp(f, r);
    fprintf(f, "=%s call $%s(", qbe_get_abi_type(&ast->expr_type), FuncCall->name);

    // Pass arguments to the function call
    int was_variadic = 0; // @HACK: 
    for (size_t i = 0; i < ArgCount; i++) {
        const ast_node_t* Expr = FuncCall->args[i];
        if (Expr->data.variable_declaration.type.kind == DATATYPE_VARIADIC){
            fprintf(f, "..., ");
            was_variadic++;
            continue;
        }

        // Account for argument promotion
        // TODO: integer promotion
        const char* arg_abi = qbe_get_abi_type(&Expr->expr_type);
        if (was_variadic && arg_abi) {
            if (strcmp(arg_abi, "s") == 0) {
                arg_abi = "d";
            }
        }

        if (arg_abi) {
            fprintf(f, "%s ", arg_abi);
        } else {
            fprintf(f, ":%s ", Expr->expr_type.typename);
        }

        fprint_temp(f, arg_temps[i-was_variadic]);
        fprintf(f, ", ");
    }
    fprintf(f, ")\n");
    arrfree(arg_temps);
    return r;
}

temporary_t qbe_generate_while_loop(
    FILE* f,
    const ast_node_t* ast,
    backend_ctx_t* ctx
) {
    DEBUG_ASSERT(ast && ast->kind == AST_WHILE_LOOP, "?");

    const ast_while_loop_t* WhileLoop = &ast->data.while_loop;

    const label_t LabelComparision = get_label();
    const label_t LabelBegin = get_label();
    const label_t LabelEnd = get_label();

    // Begin label
    fprint_label(f, LabelComparision); 
    fprintf(f, "\n");

    // Comparision
    const temporary_t CompTemp = qbe_generate_expr_node(f, WhileLoop->expr, ctx);
    fprintf(f, "\tjnz ");
    fprint_temp(f, CompTemp);
    fprintf(f, ", ");
    fprint_label(f, LabelBegin); 
    fprintf(f, ", ");
    fprint_label(f, LabelEnd); 
    fprintf(f, "\n"); 

    // Body
    fprint_label(f, LabelBegin);
    fprintf(f, "\n"); 
    const size_t BodyCount = arrlenu(WhileLoop->body);
    for (size_t i = 0; i < BodyCount; i++) {
        qbe_generate_expr_node(f, WhileLoop->body[i], ctx);
    }
    fprintf(f, "\tjmp ");
    fprint_label(f, LabelComparision); 
    fprintf(f, "\n");

    // End Label
    fprint_label(f, LabelEnd); 
    fprintf(f, "\n");

    return CompTemp;
}

temporary_t qbe_generate_if_statement(
    FILE* f,
    const ast_node_t* ast,
    backend_ctx_t* ctx
) {
    const ast_if_statement_t* IfStatement = &ast->data.if_statement;

    const label_t LabelComparision = get_label();
    const label_t LabelIf = get_label();
    const label_t LabelElse = get_label();
    const label_t LabelOut = get_label();

    fprint_label(f, LabelComparision);
    fprintf(f, "\n");
    const temporary_t CompTemp = qbe_generate_expr_node(f, IfStatement->expr, ctx);
    fprintf(f, "\tjnz ");
    fprint_temp(f, CompTemp);
    fprintf(f, ", ");
    fprint_label(f, LabelIf); 
    fprintf(f, ", ");
    fprint_label(f, LabelElse); 
    fprintf(f, "\n"); 

    // If Body
    fprint_label(f, LabelIf);
    fprintf(f, "\n");
    const size_t BodyCount = arrlenu(IfStatement->body);
    for (size_t i = 0; i < BodyCount; i++) {
        qbe_generate_expr_node(f, IfStatement->body[i], ctx);
    }
    fprintf(f, "\tjmp ");
    fprint_label(f, LabelOut);
    fprintf(f, "\n");

    // Else Body
    fprint_label(f, LabelElse);
    fprintf(f, "\n");
    const size_t ElseBodyCount = arrlenu(IfStatement->else_body);
    for (size_t i = 0; i < ElseBodyCount; i++) {
        qbe_generate_expr_node(f, IfStatement->else_body[i], ctx);
    }
    fprint_label(f, LabelOut);
    fprintf(f, "\n");
    return CompTemp;
}
