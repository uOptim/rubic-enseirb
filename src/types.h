#ifndef TYPES_H
#define TYPES_H

#include "symtable.h"

void      type_inter(struct var *, const type_t types[], int);

int       params_type_is_known(struct function *);
void      type_explicit(void *, void *, void *);

#endif /* TYPES_H */
