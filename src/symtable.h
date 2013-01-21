#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "stack.h"
#include "hashmap.h"

/* Type */
#define FUN_T   0
#define CLA_T   1
#define VAR_T   2
#define CST_T   3

#define INT_T   4
#define FLO_T   5
#define STR_T   6
#define BOO_T   7
#define OBJ_T	8

#define UND_T   127 // Undef



// symbols
struct symbol {
	char type;
	//char *name;
	void *ptr;
};

struct type {
	union {
		unsigned char t;         // general type
		struct {
			unsigned char tt:7;  // specific type
			unsigned char tc:1;  // is const
		};
	};
};

struct var {
	char *vn;

	// type
	struct stack *t;
};


#define CST_PURECST  0
#define CST_OPRESULT 1

struct cst {
	char type;
	unsigned int reg;
	union {
		int i;
		char c;
		double f;
		char *s;
	};
};

struct function {
	char *fn;
	struct var *ret;
	struct stack *params;
	struct stack *instr;
};


struct class {
	char *cn;
	struct class *super;
	struct hashmap *attrs;
	struct hashmap *methods;
};

unsigned int new_reg();

struct symbol * sym_new(const char *, char, void *);
void            sym_dump(void *);
void            sym_free(void *);

struct type * type_new(unsigned char);
void          type_free(void *);

struct var * var_new(const char *);
void         var_free(void *);
void         var_dump(void *);

struct cst * cst_new(char, char);
void         cst_free(void *);
void         cst_dump(void *);

struct class * class_new();
void           class_free(void *);
void           class_dump(void *);

struct function * function_new();
void              function_free(void *);
void              function_dump(void *);

#endif /* SYMTABLE_H */
