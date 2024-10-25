#ifndef MAYO_SEMANTICS_H
#define MAYO_SEMANTICS_H

struct ast_node_t;
struct arena_t;

// ast is not passed as constant because the type information is set for the ast node.
void semantic_analysis(struct arena_t* arena, struct ast_node_t* ast_root);

#endif
