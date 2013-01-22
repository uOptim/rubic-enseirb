#ifndef INSTR_H
#define INSTR_H

#include "symtable.h"

#define I_RET	0x00	// return
#define I_STO	0x01	// store
#define I_LOA	0x02	// load

/* Arithmetic operation, be careful if changing this */
#define I_ARI	0x10
#define I_ADD	0x11
#define I_SUB	0x12
#define I_MUL	0x13
#define I_DIV	0x14

/* Boolean operation */
#define I_BOO	0x20
#define I_EQ	0x21
#define I_NEQ	0x22
#define I_AND	0x23
#define I_OR	0x24
#define I_LEQ	0x25
#define I_GEQ	0x26
#define I_LT	0x27
#define I_GT	0x28




struct instruction;
		
void instr_push(struct function *, struct instruction *);

struct instruction * i3addr(char, struct cst *, struct cst *);

struct instruction * istore(struct var *, struct cst *);
struct instruction * iload(struct var *);

void         instruction_free(struct instruction **i);
void         instruction_dump(const struct instruction *);
struct cst * instruction_get_result(const struct instruction *);


#endif
