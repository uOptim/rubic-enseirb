#ifndef MANGLING_H
#define MANGLING_H

#define I_RET	0x00	// return
#define I_ASS	0x01	// assignment

/* Arithmetic operation, be carefull if changing this */
#define I_ARI	0x10
#define I_ADD	0x11
#define I_SUB	0x12
#define I_MUL	0x13
#define I_DIV	0x14

/* Boolean operation */
#define I_BOO	0x20

#include "symtable.h"

/* Generate the function code for each possible parameters type.
 * The code is printed on stdout.
 */
void mangle_function(struct function *);

/* s2 shall be NULL if not required for the operation
 */
struct symbol * func_push_instr(struct function *f, int op_type,
		struct symbol *s1, struct symbol *s2);

#endif /* MANGLING_H */
