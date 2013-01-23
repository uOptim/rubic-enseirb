#ifndef TYPES_H
#define TYPES_H

#include "symtable.h"

void      type_inter(struct var *, const type_t types[], int);

int       params_type_is_known(struct function *);
int       var_type_card(struct var *);
void      type_explicit(void *, void *);
type_t    var_gettype(struct var *);

#endif /* TYPES_H */
