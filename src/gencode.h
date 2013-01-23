#ifndef GENCODE_H
#define GENCODE_H

#include "stack.h"
#include "instruction.h"

int gencode_instr(struct instr *);
int gencode_stack(struct stack *);

#endif
