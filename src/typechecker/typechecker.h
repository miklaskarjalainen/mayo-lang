#ifndef MYLANG_TYPECHECKER_H
#define MYLANG_TYPECHECKER_H

#include <stdint.h>

struct ast_node_t;

typedef enum ir_instruction_kind_t {
    IR_INSTRUCTION_INVALID,
    IR_INSTRUCTION_ASSIGNMENT
} ir_instruction_kind_t;

typedef struct ir_instruction_t {
    ir_instruction_kind_t operation;
    int64_t operand1, operand2, result;
} ir_instruction_t;

ir_instruction_t* ast_to_ir(struct ast_node_t* ast);

#endif
