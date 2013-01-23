#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "stack.h"
#include "hashmap.h"

/* symbol type */
#define FUN_T   0
#define CLA_T   1
#define VAR_T   2
#define CST_T   3

#define INT_T   0
#define FLO_T   1
#define BOO_T   2
#define STR_T   3
#define OBJ_T	4

#define UND_T   127 // Undef

typedef unsigned char type_t;

extern const char compatibility_table[3][3];


struct var {
	char *vn;

	// type
	struct stack *t;
	unsigned char tt; // XXX when ok
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
	char ret;
	struct stack *params;
};


struct class {
	char *cn;
	struct class *super;
	struct hashmap *attrs;
	struct hashmap *methods;
};

unsigned int new_reg();

struct var * var_new(const char *);
void         var_free(void *);
void         var_dump(void *);
void         var_pushtype(struct var *, unsigned char);

struct cst * cst_new(char, char);
struct cst * cst_copy(struct cst *);
void         cst_free(void *);
void         cst_dump(void *);

struct class * class_new();
void           class_free(void *);
void           class_dump(void *);

struct function * function_new(const char *);
void              function_free(void *);
void              function_dump(void *);

#endif /* SYMTABLE_H */
