#ifndef MAYO_SEMANTICS_H
#define MAYO_SEMANTICS_H

struct ast_node_t;
struct arena_t;

void semantic_analysis(struct arena_t* arena, struct ast_node_t* ast_root);

#endif
