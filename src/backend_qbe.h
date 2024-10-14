#ifndef MAYO_BACKEND_QBE_H
#define MAYO_BACKEND_QBE_H

#include <stdio.h>

struct ast_node_t;

void generate_qbe(FILE* f, struct ast_node_t* ast);

#endif
