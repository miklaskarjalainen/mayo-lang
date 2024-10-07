#ifndef MYLANG_AST_PRINT_H
#define MYLANG_AST_PRINT_H

struct ast_node_t;

void print_ast_tree(const struct ast_node_t* node); // prints the contents of a node and it's children. (as a tree)

#endif
