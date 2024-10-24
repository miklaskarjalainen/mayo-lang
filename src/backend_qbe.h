#ifndef MAYO_BACKEND_QBE_H
#define MAYO_BACKEND_QBE_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct ast_node_t;
struct ast_variable_declaration_t;
struct ast_struct_declaration_t;
struct datatype_t;

typedef struct temporary_t {
    uint32_t id;
} temporary_t;
#define NULL_TEMPORARY (temporary_t){.id = 0}

typedef struct label_t {
    uint32_t id;
} label_t;

typedef struct variable_t {
    struct ast_variable_declaration_t* var_decl;
    temporary_t temp;
} variable_t;

typedef struct aggregate_type_t {
    struct ast_struct_declaration_t* ast;
} aggregate_type_t;

typedef struct backend_ctx_t {
    variable_t* variables;
    aggregate_type_t* types;
} backend_ctx_t;


temporary_t get_temporary(void);
label_t get_label(void);
void fprint_temp(FILE* f, temporary_t temp);
void fprint_label(FILE* f, label_t temp);
temporary_t qbe_generate_expr_node(FILE* f, struct ast_node_t* ast, backend_ctx_t* ctx);
void generate_qbe(FILE* f, struct ast_node_t* ast);

aggregate_type_t* qbe_find_type(const char* name, const backend_ctx_t* ctx);
size_t qbe_get_type_size(const struct datatype_t* type, const backend_ctx_t* ctx);
size_t qbe_get_aggregate_type_size(aggregate_type_t* t, const backend_ctx_t* ctx);
bool qbe_is_type_signed(const struct datatype_t* type);
size_t qbe_get_type_member_offset(const aggregate_type_t* t, const char* member_name, const backend_ctx_t* ctx);
size_t qbe_get_type_member_index(aggregate_type_t* t, const char* member_name);
char qbe_get_base_type(const struct datatype_t* register_type);         // base types: w | l | s | d
const char* qbe_get_abi_type(const struct datatype_t* register_type);   // base types + "sb" | "ub" | "sh" | "uh"
temporary_t qbe_get_ptr_with_offset(FILE* f, temporary_t begin_ptr, temporary_t offset);
temporary_t qbe_get_array_ptr(
    FILE* f,
    temporary_t array_ptr,
    temporary_t index_temp,
    const struct datatype_t* element_type,
    const backend_ctx_t* ctx
);
variable_t* qbe_find_variable(const char* name, backend_ctx_t* ctx);
const char* qbe_get_store_ins(const struct datatype_t* register_type);
const char* qbe_get_load_ins(const struct datatype_t* register_type);

#endif
