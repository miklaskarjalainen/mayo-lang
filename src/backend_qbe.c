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

static label_t get_label(void) {
    static uint32_t s_Id = 0;
    return (label_t){.id = s_Id++};
}

static void fprint_temp(FILE* f, temporary_t temp) {
    fprintf(f, "%%r%u", temp.id);
} 

static void fprint_label(FILE* f, label_t temp) {
    fprintf(f, "@l%u", temp.id);
} 

static size_t _get_type_size(const datatype_t* type) {
    /*
        Cheatsheet:
            Byte is size of 1. So:
            (b)yte      = 1 (8-bit),
            (h)alf word = 2 (16-bit),
            (w)ord      = 4 (32-bit),
            (l)ong/ptr  = 8 (64-bit),
            
            (s)ingle    = 4 (32-bit),
            (d)ouble    = 8 (64-bit),
    */
    
    switch (type->kind) {
        case DATATYPE_POINTER: {
            return 8;
        }

        case DATATYPE_ARRAY: {
            const size_t ElementSize = _get_type_size(type->base);
            return ElementSize * type->array_size;
        }

        case DATATYPE_PRIMITIVE: {
#define IF_TYPE_RET(s1, ret) if (strcmp(type->typename, s1) == 0) { return ret; } 
            IF_TYPE_RET("char", 1);
            IF_TYPE_RET("i8", 1);
            IF_TYPE_RET("u8", 1);
            IF_TYPE_RET("i16", 2);
            IF_TYPE_RET("u16", 2);
            IF_TYPE_RET("i32", 4);
            IF_TYPE_RET("u32", 4);
            IF_TYPE_RET("i64", 8);
            IF_TYPE_RET("u64", 8);

            IF_TYPE_RET("f32", 4);
            IF_TYPE_RET("f64", 8);
#undef IF_TYPE_RET
            PANIC("Size not implemented for type '%s'", type->typename);
        }

        default: {
            UNIMPLEMENTED("Invalid type");
        }
    }

    return 0;
}

static const char* _get_store_ins(const datatype_t* register_type) {
    switch (register_type->kind) {
        case DATATYPE_ARRAY:
        case DATATYPE_POINTER: {
            return "storel";
        }

        case DATATYPE_PRIMITIVE: {
#define IF_TYPE_RET(s1, ret) if (strcmp(register_type->typename, s1) == 0) { return ret; } 
            IF_TYPE_RET("char", "storeb");
            IF_TYPE_RET("i8", "storeb");
            IF_TYPE_RET("u8", "storeb");
            IF_TYPE_RET("i16", "storeh");
            IF_TYPE_RET("u16", "storeh");
            IF_TYPE_RET("i32", "storew");
            IF_TYPE_RET("u32", "storew");
            IF_TYPE_RET("i64", "storel");
            IF_TYPE_RET("u64", "storel");

            IF_TYPE_RET("f32", "stores");
            IF_TYPE_RET("f64", "stored");
#undef IF_TYPE_RET
            PANIC("Size not implemented for type '%s'", register_type->typename);
        }

        default: {
            UNIMPLEMENTED("Invalid type");
        }
    }

    return 0;
}

static const char* _get_load_ins(const datatype_t* register_type) {
    switch (register_type->kind) {
        case DATATYPE_ARRAY:
        case DATATYPE_POINTER: {
            return "loadl";
        }

        case DATATYPE_PRIMITIVE: {
#define IF_TYPE_RET(s1, ret) if (strcmp(register_type->typename, s1) == 0) { return ret; } 
            IF_TYPE_RET("char", "=w loadub");
            IF_TYPE_RET("i8"  , "=w loadsb");
            IF_TYPE_RET("u8"  , "=w loadub");
            IF_TYPE_RET("i16" , "=w loadsh");
            IF_TYPE_RET("u16" , "=w loaduh");
            IF_TYPE_RET("i32" , "=w loadsw");
            IF_TYPE_RET("u32" , "=w loaduw");
            IF_TYPE_RET("i64" , "=l loadl");
            IF_TYPE_RET("u64" , "=l loadl");

            IF_TYPE_RET("f32" , "=s loads");
            IF_TYPE_RET("f64" , "=d loadd");
#undef IF_TYPE_RET
            PANIC("Size not implemented for type '%s'", register_type->typename);
        }

        default: {
            UNIMPLEMENTED("Invalid type");
        }
    }

    return 0;
}

static temporary_t _get_array_ptr(FILE* f, temporary_t array_ptr, temporary_t index_temp, const datatype_t* element_type) {
    // @FIXME: Casts a temporary to long, even thought it already might be :)
    // Index operator: e.g
    // %ptr =l add %arr_begin, (size_of*i)
    
    // Casted index
    temporary_t casted_index = get_temporary();
    fprintf(f, "\t");
    fprint_temp(f, casted_index);
    fprintf(f, "=l extsw "); //! 'extsw' converts a signed word to a long here.
    fprint_temp(f, index_temp);

    // Multiply index with size of an element in the array
    const size_t ElementSize = _get_type_size(element_type);
    if (ElementSize > 1) {
        fprintf(f, "\n\t");
        fprint_temp(f, casted_index);
        fprintf(f, "=l mul %zu, ", ElementSize);
        fprintf(f, ", ");
        fprint_temp(f, casted_index);
    }

    // Get address for index
    temporary_t ptr = get_temporary();
    fprintf(f, "\n\t");
    fprint_temp(f, ptr);
    fprintf(f, "=l add ");
    fprint_temp(f, array_ptr);
    fprintf(f, ", ");
    fprint_temp(f, casted_index);
    fprintf(f, " # Index nth\n");

    return ptr;
}

static size_t _find_variable(const char* name, qbe_variable_t** variables) {
    // Search for temp
    size_t VarLen = arrlenu(variables);
    for (size_t i = 0; i < VarLen; i++) {
        if (strcmp((*variables)[i].var_name, name) == 0) {
            return i;
        }
    }

    return -1;
}

static temporary_t _generate_expr_node(FILE* f, ast_node_t* ast, qbe_variable_t** variables) {
    switch (ast->kind) {
        // @FIXME: Actually implement this.
        case AST_CAST_STATEMENT: {
            return _generate_expr_node(f, ast->data.cast_statement.expr, variables);
        }

        case AST_UNARY_OP: {
            return _generate_expr_node(f, ast->data.unary_op.operand, variables); 
        }

        case AST_CHAR_LITERAL: {
            temporary_t r = get_temporary();
            fprintf(f, "\t");
            fprint_temp(f, r);
            fprintf(f, "=w copy %u\n", ast->data.c);
            return r;
        }

        case AST_INTEGER_LITERAL: {
            temporary_t r = get_temporary();
            fprintf(f, "\t");
            fprint_temp(f, r);
            fprintf(f, "=w copy %li\n", ast->data.integer);
            return r;
        }

        case AST_STRING_LITERAL: {
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

        case AST_ARRAY_INITIALIZER_LIST: {
            const datatype_t* ExprType = &ast->expr_type;
            DEBUG_ASSERT(ExprType->kind == DATATYPE_ARRAY, "?");

            const size_t ElementSize = _get_type_size(ExprType);
            const size_t ArraySize = arrlenu(ast->data.array_initializer_list.exprs);
            const size_t AllocSize = ArraySize * ElementSize;

            // Allocate storage for the string
            const temporary_t ArrayBegin  = get_temporary();
            fprintf(f, "\t");
            fprint_temp(f, ArrayBegin);
            fprintf(f, "=l alloc4 %zu\n", AllocSize);

            // Store the characters from the string to the array
            for (size_t i = 0; i < ArraySize; i++) {
                // Make a pointer to the index
                const temporary_t IndexPtr  = get_temporary();
                fprintf(f, "\t");
                fprint_temp(f, IndexPtr);
                fprintf(f, " =l add ");
                fprint_temp(f, ArrayBegin);
                fprintf(f, ", %zu # Array[%zu] \n", i, i);

                // Get expr
                fprintf(f, "# Array[%zu] expr \n", i);
                const temporary_t ValueTemp  = _generate_expr_node(f, ast->data.array_initializer_list.exprs[i], variables);

                // Store a byte into index
                fprintf(f, "\t%s ", _get_store_ins(ExprType->base));
                fprint_temp(f, ValueTemp);
                fprintf(f, ", ");
                fprint_temp(f, IndexPtr);
                fprintf(f, "\n");
            }

            return ArrayBegin;
        }

        case AST_GET_VARIABLE: {
            // Search for temp
            const size_t Place = _find_variable(ast->data.literal, variables);
            if (Place != (size_t)-1) {
                return (*variables)[Place].temp;
            }
            PANIC("no temporary for variable '%s'!", ast->data.literal);
            break;
        }

        case AST_BINARY_OP: {
            if (ast->data.binary_op.operation == BINARY_OP_ASSIGN) {
                const ast_node_t* Lhs = ast->data.binary_op.left;
                DEBUG_ASSERT(
                    Lhs->kind == AST_GET_VARIABLE || 
                    (Lhs->kind == AST_BINARY_OP && Lhs->data.binary_op.operation == BINARY_OP_ARRAY_INDEX),
                    "Invalid assignment!"
                );

                const temporary_t Temp = _generate_expr_node(f, ast->data.binary_op.right, variables);
                if (Lhs->kind == AST_BINARY_OP && Lhs->data.binary_op.operation == BINARY_OP_ARRAY_INDEX) {
                    // Get ptr to index
                    const ast_node_t* ArrayIndexAst = ast->data.binary_op.left;
                    const datatype_t ElementType = ArrayIndexAst->expr_type;
                    temporary_t array_temp = _generate_expr_node(f, ArrayIndexAst->data.binary_op.left, variables);
                    temporary_t index_temp = _generate_expr_node(f, ArrayIndexAst->data.binary_op.right, variables);
                    temporary_t ptr_temp = _get_array_ptr(f, array_temp, index_temp, &ElementType);

                    // Eval expr
                    temporary_t expr_temp = _generate_expr_node(f, ast->data.binary_op.right, variables);
                    
                    // Store
                    fprintf(f, "\t%s ", _get_store_ins(&ElementType));
                    fprint_temp(f, expr_temp);
                    fprintf(f, ", ");
                    fprint_temp(f, ptr_temp);
                    fprintf(f, "\n");
                    return ptr_temp;
                }
                else {
                    const char* VarName = ast->data.binary_op.left->data.literal;
                    const size_t Place = _find_variable(VarName, variables);
                    if (Place == (size_t)-1) {
                        PANIC("no temporary for variable '%s'!", VarName);
                    }
                    fprintf(f, "\t");
                    fprint_temp(f, (*variables)[Place].temp);
                }

                fprintf(f, "=w copy ");
                fprint_temp(f, Temp);
                fprintf(f, "\n");
                return Temp;
            }

            temporary_t lhs = _generate_expr_node(f, ast->data.binary_op.left, variables);
            temporary_t rhs = _generate_expr_node(f, ast->data.binary_op.right, variables);

            char* qbe_operation = NULL;
            switch (ast->data.binary_op.operation) {
                case BINARY_OP_ADD      : { qbe_operation = "=w add"; break; }
                case BINARY_OP_SUBTRACT : { qbe_operation = "=w sub "; break; }
                case BINARY_OP_MULTIPLY : { qbe_operation = "=w mul "; break; }
                case BINARY_OP_EQUAL    : { qbe_operation = "=w ceqw "; break; }
                case BINARY_OP_NOT_EQUAL: { qbe_operation = "=w cnew "; break; }

                case BINARY_OP_ASSIGN   : { qbe_operation = "=w "; break; }

                case BINARY_OP_ARRAY_INDEX: {
                    temporary_t ptr = _get_array_ptr(f, lhs, rhs, &ast->expr_type);

                    // Get memory from that index
                    temporary_t result = get_temporary();
                    fprintf(f, "\t");
                    fprint_temp(f, result);
                    fprintf(f, "%s ", _get_load_ins(&ast->expr_type));
                    fprint_temp(f, ptr);
                    fprintf(f, "# indexed element\n");

                    return result;
                }

                default: {
                    PANIC("Op not implemented %u", ast->data.binary_op.operation);
                }
            }

            temporary_t r = get_temporary();
            fprintf(f, "\t");
            fprint_temp(f, r);
            fprintf(f, "%s", qbe_operation);
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
            arrput(*variables, VarTemp);
            return r;
        }
        

        case AST_WHILE_LOOP: {
            const ast_while_loop_t* WhileLoop = &ast->data.while_loop;

            const label_t LabelComparision = get_label();
            const label_t LabelBegin = get_label();
            const label_t LabelEnd = get_label();

            // Begin label
            fprint_label(f, LabelComparision); 
            fprintf(f, "\n");

            // Comparision
            const temporary_t CompTemp = _generate_expr_node(f, WhileLoop->expr, variables);
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
                _generate_expr_node(f, WhileLoop->body[i], variables);
            }
            fprintf(f, "\tjmp ");
            fprint_label(f, LabelComparision); 
            fprintf(f, "\n");

            // End Label
            fprint_label(f, LabelEnd); 
            fprintf(f, "\n");

            return CompTemp;
        }

        case AST_IF_STATEMENT: {
            const ast_if_statement_t* IfStatement = &ast->data.if_statement;

            const label_t LabelComparision = get_label();
            const label_t LabelIf = get_label();
            const label_t LabelElse = get_label();
            const label_t LabelOut = get_label();

            fprint_label(f, LabelComparision);
            fprintf(f, "\n");
            const temporary_t CompTemp = _generate_expr_node(f, IfStatement->expr, variables);
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
                _generate_expr_node(f, IfStatement->body[i], variables);
            }
            fprintf(f, "\tjmp ");
            fprint_label(f, LabelOut);
            fprintf(f, "\n");

            // Else Body
            fprint_label(f, LabelElse);
            fprintf(f, "\n");
            const size_t ElseBodyCount = arrlenu(IfStatement->else_body);
            for (size_t i = 0; i < ElseBodyCount; i++) {
                _generate_expr_node(f, IfStatement->else_body[i], variables);
            }
            fprint_label(f, LabelOut);
            fprintf(f, "\n");
            return CompTemp;
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
            if (FuncDecl->external) {
                return;
            }
            
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
                _generate_expr_node(f, FuncDecl->body[i], &variables);
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
