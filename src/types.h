#ifndef TYPES_H
#define TYPES_H

#include "symtable.h"

void          type_inter(struct var *, const unsigned char types[], int);

int           params_type_is_known(struct function *);
int           var_type_card(struct var *);
static void   type_explicit(void *, void *);
unsigned char var_type(struct var *);

#endif /* TYPES_H */
