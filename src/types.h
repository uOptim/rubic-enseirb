#ifndef TYPES_H
#define TYPES_H

#include "symtable.h"

void      types_init(struct stack *);
type_t    types_new(void);
void      var_put_types(struct var *, const struct stack *);

int       params_type_is_known(struct function *);
void      type_inter(struct var *, const type_t types[], int);

#endif /* TYPES_H */
