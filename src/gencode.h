#ifndef GENCODE_H
#define GENCODE_H

#include "symtable.h"
#include "stack.h"
#include "instruction.h"


int gencode_instr(struct instr *);
int gencode_stack(struct stack *);
void gencode_main(struct stack *);
void gencode_func(struct function *, const char *, struct stack *);

const char * func_mangling(struct function *);

#endif
