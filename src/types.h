#ifndef TYPES_H
#define TYPES_H

#include "symtable.h"

void      type_init(struct stack *);
type_t    type_new(void);
void      var_put_types(struct var *, struct stack *);

int       params_type_is_known(struct function *);
struct stack * type_inter(struct stack *, struct stack *);

#endif /* TYPES_H */
