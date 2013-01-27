#ifndef TYPES_H
#define TYPES_H

#include "symtable.h"

#define TYPE_NB	4

extern type_t possible_types[TYPE_NB];

void           type_init(struct stack *);
type_t         type_new(void);
void           var_put_types(struct var *, struct stack *);

int            type_ispresent(struct stack *, type_t);
struct stack * type_inter(struct stack *, struct stack *);
struct stack * type_union(struct stack *, struct stack *);

// used when copying a stack of types
void * type_copy(void *);

#endif /* TYPES_H */
