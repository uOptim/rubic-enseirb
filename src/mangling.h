#ifndef MANGLING_H
#define MANGLING_H

#include "symtable.h"
#include "instruction.h"

/* Generate the function code for each possible parameters type.
 * The code is printed on stdout.
 */
void func_gen_codes(struct function *);

#endif /* MANGLING_H */
