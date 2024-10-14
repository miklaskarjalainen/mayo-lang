#include <stb/stb_ds.h>
#include <stdint.h>
#include <stdio.h>

#include "../common/error.h"
#include "../common/utils.h"

#include "ast_print.h"
#include "ast_kinds.h"
#include "ast_type.h"


/*
    Usage:
        These allows drawing the pipes past grand children.
        For the last child the branch should be cleared so "BRANCH_LAST_CHILD_CHAR" can be printed.
*/
#define HAS_BRANCH(depth)   (bool)(sPrintTreeBranchBits & (1 << (depth)))
#define SET_BRANCH(depth)   (sPrintTreeBranchBits |= 1 << (depth))
#define CLEAR_BRANCH(depth) (sPrintTreeBranchBits &= ~(1 << (depth)))

#define AST_TYPE_COLOR STDOUT_BLUE
#define AST_SECONDARY_COLOR STDOUT_RED
#define BRANCH_COLOR STDOUT_GREEN
#define BRANCH_CHAR "\u2503"
#define BRANCH_CHILD_CHAR "\u2523"
#define BRANCH_LAST_CHILD_CHAR "\u2517"
#define BRANCH_CONNECT_CHILD_CHAR "\u2501"

static uint64_t sPrintTreeBranchBits = 0;

const char* op_to_str(op_t op) {
    DEBUG_ASSERT(op >= 0 && op < OP_COUNT, "op is out of range");
    static const char* sOp2Str[] = {
        #define _DEF(internal) #internal
        #include "ops.h"
        #undef _DEF
    };
    return sOp2Str[op];
}

const char* ast_kind_to_str(ast_kind_t kind) {
    DEBUG_ASSERT(kind >= 0 && kind < AST_COUNT, "ast kind is out of range");
    static const char* sAst2Str[] = {
        #define _DEF(internal) #internal
        #include "ast_kinds.h"
        #undef _DEF
    };
    return sAst2Str[kind];    
}

static void print_ast_indention(size_t depth) {
    for (size_t i = 0; i < depth; i++) {
        if (HAS_BRANCH(i)) {
            /* parent of the child */
            if ((i+1) == depth) {
                printf(BRANCH_COLOR "" BRANCH_CHILD_CHAR BRANCH_CONNECT_CHILD_CHAR " " STDOUT_RESET);
            }
            /* not a parent of the child */
            else {
                printf(BRANCH_COLOR "" BRANCH_CHAR"  " STDOUT_RESET);
            }
            continue;
        }

        /* last child of the parent */
        if ((i+1) == depth) {
            printf(BRANCH_COLOR "" BRANCH_LAST_CHILD_CHAR BRANCH_CONNECT_CHILD_CHAR " " STDOUT_RESET);
            continue;
        }

        print_spaces(3);
    }
}

/*
    Print func for every type. I do this to force type safety, this could just be a HUGE switch statement.
*/
#define SPACE_MULTIPLIER 3
#define AST_PRINT(depth, ...)       \
    do {                            \
        print_ast_indention(depth); \
        printf(__VA_ARGS__);        \
    } while (0)

#define AST_PRINT_SETUP(depth, kind, ...)       \
    do {                                        \
        print_ast_indention(depth);             \
        printf(AST_TYPE_COLOR "%s " STDOUT_RESET, ast_kind_to_str(kind));   \
        printf(__VA_ARGS__);                    \
    } while (0)

#define AST_PRINT_SETUP_NO_ADDITIONAL(depth, kind) \
    AST_PRINT_SETUP(depth, kind, "\n")

#define AST_PRINT_NODE_BODY(array, depth)  \
    do {                                                        \
        SET_BRANCH(depth);                                      \
        const size_t Len = arrlenu(array);                      \
        for (size_t i = 0; i < Len; i++) {                      \
            if ((i+1) == Len) { CLEAR_BRANCH(depth); }          \
            print_ast_internal(array[i], depth+1);              \
        }                                                       \
    } while(0)

#define PRINT_POS(pos) printf("<%s:%zu:%zu:%i>", pos.filepath, pos.line, pos.column, pos.length);

static void print_ast_internal(const ast_node_t* node, size_t depth);

static void print_ast_unary_op(const ast_unary_op_t* unary_op, size_t depth) {
    AST_PRINT_SETUP(depth, AST_UNARY_OP, "<%s>\n", op_to_str(unary_op->operation));

    print_ast_internal(unary_op->operand, depth+1);
}

static void print_ast_binary_op(const ast_binary_op_t* binary_op, size_t depth) {
    AST_PRINT_SETUP(depth, AST_BINARY_OP, "<%s>\n", op_to_str(binary_op->operation));

    SET_BRANCH(depth);
    print_ast_internal(binary_op->left, depth+1);
    CLEAR_BRANCH(depth);
    print_ast_internal(binary_op->right, depth+1);
}


static void print_ast_if_statement(const ast_if_statement_t* if_statement, size_t depth) {
    AST_PRINT_SETUP_NO_ADDITIONAL(depth, AST_IF_STATEMENT);

    SET_BRANCH(depth);
    AST_PRINT(depth+1, "if-expr: \n");
    print_ast_internal(if_statement->expr, depth+2);

    if (!if_statement->else_body) { CLEAR_BRANCH(depth); }

    SET_BRANCH(depth+1);
    AST_PRINT(depth+1, "if-body: \n");
    AST_PRINT_NODE_BODY(if_statement->body, depth+1);

    if (if_statement->else_body) {
        CLEAR_BRANCH(depth);
        AST_PRINT(depth+1, "else-body: \n");
        SET_BRANCH(depth+1);
        AST_PRINT_NODE_BODY(if_statement->else_body, depth+1);
    }
}

static void print_ast_variable_declaration(const ast_variable_declaration_t* var_decl, size_t depth) {
    AST_PRINT_SETUP(depth, AST_VARIABLE_DECLARATION, "<%s: ", var_decl->name);
    datatype_print(&var_decl->type);
    printf(">\n");

    print_ast_internal(var_decl->expr, depth+1);
}

static void print_ast_field_initializer(const ast_field_initializer_t* field_init, size_t depth) {
    AST_PRINT_SETUP(depth, AST_FIELD_INITIALIZER, "<%s>\n", field_init->name);

    print_ast_internal(field_init->expr, depth+1);
}

static void print_ast_function_declaration(const ast_function_declaration_t* func_decl, size_t depth) {
    AST_PRINT_SETUP(depth, AST_FUNCTION_DECLARATION, "<%s> <(", func_decl->name);
    /* Args */
    const size_t ArgCount = arrlenu(func_decl->args);
    for (size_t i = 0; i < ArgCount; i++) {
        if (i != 0) {
            printf(", ");
        }
        const ast_variable_declaration_t* Arg = &func_decl->args[i].data.variable_declaration;
        printf("%s: ", Arg->name);
        datatype_print(&Arg->type);
    }
    /* Return type */
    printf(")> -> <");
    datatype_print(&func_decl->return_type);
    printf(">\n");

    AST_PRINT_NODE_BODY(func_decl->body, depth);
}

static void print_ast_struct_declaration(const ast_struct_declaration_t* struct_decl, size_t depth) {
    AST_PRINT_SETUP(depth, AST_STRUCT_DECLARATION, "<%s> <{", struct_decl->name);
    /* Members */
    const size_t MemberCount = arrlenu(struct_decl->members);
    for (size_t i = 0; i < MemberCount; i++) {
        if (i != 0) {
            printf(", ");
        }
        
        const ast_variable_declaration_t* Field = &struct_decl->members[i];
        printf("%s: ", Field->name);
        datatype_print(&Field->type);
    }
    printf("}>\n");
}

static void print_ast_struct_initializer_list(const ast_struct_initializer_list_t* init_list, size_t depth) {
    AST_PRINT_SETUP(depth, AST_STRUCT_INITIALIZER_LIST, "<%s>\n", init_list->name);
    /* Fields */
    SET_BRANCH(depth);

    // AST_PRINT_NODE_BODY(init_list->fields);
    const size_t FieldCount = arrlenu(init_list->fields);
    for (size_t i = 0; i < FieldCount; i++) {
        if ((i+1) == FieldCount) { CLEAR_BRANCH(depth); }

        const ast_field_initializer_t* Field = &init_list->fields[i];
        print_ast_field_initializer(Field, depth + 1);
    }
}

static void print_ast_array_initializer_list(const ast_array_initializer_list_t* init_list, size_t depth) {
    AST_PRINT_SETUP_NO_ADDITIONAL(depth, AST_ARRAY_INITIALIZER_LIST);
    /* Fields */
    SET_BRANCH(depth);
    const size_t ExprsCount = arrlenu(init_list->exprs);
    for (size_t i = 0; i < ExprsCount; i++) {
        if ((i+1) == ExprsCount) { CLEAR_BRANCH(depth); }

        const ast_node_t* Expr = init_list->exprs[i];
        print_ast_internal(Expr, depth + 1);
    }
}

static void print_ast_function_call(const ast_function_call_t* func_call, size_t depth) {
    AST_PRINT_SETUP(depth, AST_FUNCTION_CALL, "<%s>\n", func_call->name);
    /* Args */
    SET_BRANCH(depth);
    const size_t ArgCount = arrlenu(func_call->args);
    for (size_t i = 0; i < ArgCount; i++) {
        if ((i+1) == ArgCount) { CLEAR_BRANCH(depth); }

        const ast_node_t* Arg = func_call->args[i];
        print_ast_internal(Arg, depth + 1);
    }
}

static void print_ast_while_loop(const ast_while_loop_t* while_loop, size_t depth) {
    AST_PRINT_SETUP_NO_ADDITIONAL(depth, AST_WHILE_LOOP);
    
    if (while_loop->body)
        SET_BRANCH(depth);
    
    print_ast_internal(while_loop->expr, depth + 1);
    AST_PRINT_NODE_BODY(while_loop->body, depth);
}

static void print_ast_for_loop(const ast_for_loop_t* for_loop, size_t depth) {
    AST_PRINT_SETUP(depth, AST_FOR_LOOP, "<%s> <%zu..%zu>\n", for_loop->identifier, for_loop->iter.from, for_loop->iter.to);
    AST_PRINT_NODE_BODY(for_loop->body, depth);
}

static void print_ast_translation_unit(const ast_translation_unit_t* unit, size_t depth) {
    AST_PRINT_SETUP_NO_ADDITIONAL(depth, AST_TRANSLATION_UNIT);
    AST_PRINT_NODE_BODY(unit->body, depth);
}

static void print_ast_literal(const ast_node_t* node, size_t depth) {
    AST_PRINT_SETUP(depth, node->kind, "<%s> ", node->data.literal);
    PRINT_POS(node->position);
    printf("\n");
}

static void print_ast_constant(const ast_node_t* node, size_t depth) {
    AST_PRINT_SETUP(depth, node->kind, "<");
    variant_print(&node->data.constant);
    printf("> ");
    PRINT_POS(node->position);
    printf("\n");
}

static void print_return(const ast_node_t* node, size_t depth) {
    AST_PRINT_SETUP_NO_ADDITIONAL(depth, AST_RETURN);
    print_ast_internal(node->data.expr, depth + 1);
}

static void print_integer_literal(const ast_node_t* node, size_t depth) {
    AST_PRINT_SETUP(depth, node->kind, AST_SECONDARY_COLOR "'%li' " STDOUT_RESET , node->data.integer);
    PRINT_POS(node->position);
    printf("\n");
}

static void print_basic(const ast_node_t* node, size_t depth) {
    AST_PRINT_SETUP(depth, node->kind, " ");
    PRINT_POS(node->position);
    printf("\n");
}

static void print_ast_internal(const ast_node_t* node, size_t depth) {
    if (node == NULL) {
        AST_PRINT_SETUP(depth, AST_NONE, "<null>\n");
        return;
    }

    switch(node->kind) {
        /* ast types */
        case AST_STRUCT_INITIALIZER_LIST : print_ast_struct_initializer_list(&node->data.struct_initializer_list, depth); break;
        case AST_ARRAY_INITIALIZER_LIST  : print_ast_array_initializer_list(&node->data.array_initializer_list, depth); break;
        case AST_TRANSLATION_UNIT        : print_ast_translation_unit    (&node->data.translation_unit, depth); break;
        case AST_FIELD_INITIALIZER       : print_ast_field_initializer   (&node->data.field_initializer, depth); break;
        case AST_VARIABLE_DECLARATION    : print_ast_variable_declaration(&node->data.variable_declaration, depth); break;
        case AST_STRUCT_DECLARATION      : print_ast_struct_declaration  (&node->data.struct_declaration, depth); break;
        case AST_FUNCTION_DECLARATION    : print_ast_function_declaration(&node->data.function_declaration, depth); break;
        case AST_FUNCTION_CALL           : print_ast_function_call       (&node->data.function_call, depth); break;
        case AST_WHILE_LOOP              : print_ast_while_loop          (&node->data.while_loop, depth); break;
        case AST_FOR_LOOP                : print_ast_for_loop            (&node->data.for_loop, depth); break;
        case AST_IF_STATEMENT            : print_ast_if_statement        (&node->data.if_statement, depth); break;
        case AST_UNARY_OP                : print_ast_unary_op            (&node->data.unary_op, depth); break;
        case AST_BINARY_OP               : print_ast_binary_op           (&node->data.binary_op, depth); break;
        case AST_RETURN                  : print_return(node, depth); break;

        
        /* generic */
        case AST_IMPORT      : print_ast_literal(node, depth); break;
        case AST_GET_VARIABLE: print_ast_literal(node, depth); break;
        case AST_STRING_LITERAL: print_ast_literal(node, depth); break;
        case AST_CONST_VALUE : print_ast_constant(node, depth); break;
        case AST_INTEGER_LITERAL : print_integer_literal(node, depth); break;

        /* basic */
        case AST_BREAK   : print_basic(node, depth); break;
        case AST_CONTINUE: print_basic(node, depth); break;

        default: {
            PANIC("ast printing not implemented for type %s", ast_kind_to_str(node->kind));
            break;
        }
    }
}

void print_ast_tree(const ast_node_t* node) {
    DEBUG_ASSERT(node, "node is null");
    print_ast_internal(node, 0);
}
