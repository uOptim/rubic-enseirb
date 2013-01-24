#ifndef INSTR_H
#define INSTR_H

#include "symtable.h"

#define I_RET	0x00	// return
#define I_STO	0x01	// store
#define I_LOA	0x02	// load
#define I_ALO	0x03	// allocate

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
#define I_CMP	0x30	// operations below are comparative operations
#define I_EQ	0x31
#define I_NEQ	0x32
#define I_LEQ	0x33
#define I_GEQ	0x34
#define I_LT	0x35
#define I_GT	0x36

/* raw llvm instruction */
#define I_RAW	0x80


struct symbol {
	unsigned char type;
	union {
		struct cst *cst;
		struct var *var;
	};
};

struct instr {
	unsigned char op_type;

	union {
		char *rawllvm;
		struct {
			struct symbol * sr; // returned symbol
			struct symbol * s1;
			struct symbol * s2; // might be unused for some instruction
		};
	};
};

		
struct instr * iret(struct cst *);
struct instr * iload(struct var *);
struct instr * iraw(const char *s);
struct instr * ialloca(struct var *);
struct instr * istore(struct var *, struct cst *);
struct instr * i3addr(char, struct cst *, struct cst *);

void instr_free(void *);
void instr_dump(const struct instr *);

struct cst * instr_get_result(const struct instr *);

void type_constrain(struct instr *);

/* Print an instrcode on stdout.
 * Symbol type needs to be determined especially for variables.
 */
void instr_print(struct instr*);

#endif
