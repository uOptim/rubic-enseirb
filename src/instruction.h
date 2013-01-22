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




struct instruction;
		
void instr_push(struct function *, struct instruction *);

struct instruction * i_add(struct cst *, struct cst *, struct cst *);
struct instruction * i_sub(struct cst *, struct cst *, struct cst *);
struct instruction * i_mul(struct cst *, struct cst *, struct cst *);
struct instruction * i_div(struct cst *, struct cst *, struct cst *);

struct instruction * i_store(struct var *, struct cst *);
struct instruction * i_load(struct var *, struct cst *);

struct instruction * i_assign(struct var *, struct cst *);


#endif
