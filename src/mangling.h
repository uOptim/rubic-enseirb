#ifndef MANGLING_H
#define MANGLING_H

#include "symtable.h"
#include "instruction.h"


/* Print an instruction code on stdout.
 * Symbol type needs to be determined especially for variables.
 */
void instr_print(int op_type, struct symbol *s1, struct symbol *s2);


/* Generate the function code for each possible parameters type.
 * The code is printed on stdout.
 */
void func_gen_codes(struct function *);

#endif /* MANGLING_H */
