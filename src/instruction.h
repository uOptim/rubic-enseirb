#ifndef INSTR_H
#define INSTR_H

#include "symtable.h"

#define I_RET	0x00	// return
#define I_STO	0x01	// store
#define I_LOA	0x02	// load
#define I_ALO	0x03	// allocate
#define I_CAL	0x04	// call function
#define I_PUT	0x05	// puts

/* Arithmetic operation, be careful if changing this */
#define I_ARI	0x10
#define I_ADD	0x11
#define I_SUB	0x12
#define I_MUL	0x13
#define I_DIV	0x14

/* Boolean operation */
#define I_BOO	0x20	// operations below return boolean
#define I_AND	0x21
#define I_OR	0x22

/* comparison operations : ari -> bool */
#define I_CMP	0x40	// operations below are comparative operations
#define I_EQ	0x41
#define I_NEQ	0x42
#define I_LEQ	0x43
#define I_GEQ	0x44
#define I_LT	0x45
#define I_GT	0x46

/* raw llvm instruction */
#define I_RAW	0x80


struct instr {
	unsigned char optype;

	union {
		char *rawllvm;

		struct {
			struct var * vr;
			struct elt * er; // returned symbol
			struct elt * e1;
			struct elt * e2; // might be unused for some instruction
		};
	};
};

		
struct instr * iputs(struct elt *);
struct instr * iret(struct elt *);
struct instr * iload(struct var *);
struct instr * iraw(const char *s);
struct instr * ialloca(struct var *);
struct instr * istore(struct var *, struct elt *);
struct instr * i3addr(char, struct elt *, struct elt *);

void * instr_copy(void *);
void   instr_free(void *);
void   instr_constrain(void *, void *, void *);

struct elt * instr_get_result(const struct instr *);

#endif
