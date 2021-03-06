#ifndef MANGLING_H
#define MANGLING_H

#include "symtable.h"
#include "stack.h"
#include "hashmap.h"

/* Generate the function code for each possible parameters type.
 * The code is printed on stdout.
 */
void func_gen_codes(struct function *, struct stack *, struct hashmap *);

#endif /* MANGLING_H */
