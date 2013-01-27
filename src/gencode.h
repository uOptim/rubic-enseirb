#ifndef GENCODE_H
#define GENCODE_H

#include "symtable.h"
#include "stack.h"
#include "instruction.h"


void gencode_main(struct stack *, struct hashmap *);
void gencode_func(struct function *, const char *,
		struct stack *, struct hashmap *);

const char * func_mangling(struct function *);

void casttobool(struct elt *, struct elt **);

#endif
