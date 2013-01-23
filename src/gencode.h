#ifndef GENCODE_H
#define GENCODE_H

#include "symtable.h"
#include "stack.h"
#include "instruction.h"

int gencode_instr(struct instr *);
int gencode_stack(struct stack *);
void gencode_func(struct function *, struct stack *);

void print_instr(struct instr *);

#endif
