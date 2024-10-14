#ifndef MYLANG_PARSER_PARSE_H
#define MYLANG_PARSER_PARSE_H
#include <stdbool.h>

struct parser_t;
struct ast_node_t;

struct ast_node_t* parse_struct_initializer_list(struct parser_t* parser, const char* type_identifier);
struct ast_node_t* parse_array_initializer_list(struct parser_t* parser);
struct ast_node_t* parse_function_call(struct parser_t* parser, const char* identifier, bool expr);
struct ast_node_t* parse_cast_statement(struct parser_t* parser);
struct ast_node_t* parse_import_statement(struct parser_t* parser);
struct ast_node_t* parse_variable_declaration(struct parser_t* parser);
struct ast_node_t** parse_global_scope(struct parser_t* parser);
struct ast_node_t** parse_body(struct parser_t* parser);

#endif
