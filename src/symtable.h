#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "stack.h"
#include "hashmap.h"

#define INT_T   0
#define FLO_T   1
#define BOO_T   2
#define STR_T   3
#define OBJ_T	4

#define UND_T   127 // Undef

/* symbol type */
#define FUN_T   0
#define CLA_T   1
#define VAR_T   2
#define CST_T   3

#define DUMMY_FUNC ((void *)0x01)

typedef unsigned int type_t;

extern const char compatibility_table[3][3];


struct var {
	char *vn;
	char is_params;  // tells if the variable is a function parameter
	struct stack *t; // possible types
};


struct var * var_new(const char *);
struct var * var_copy(struct var *);
void         var_free(void *);
type_t       var_gettype(struct var *);
int          var_type_card(struct var *);
int          var_isconst(const struct var *);
int          var_isglobal(const struct var *);


struct reg {
	char bound;
	unsigned int num;
	struct stack *types; // NEVER FREE THE CONTENT!
};

struct reg * reg_new(struct var *);
void         reg_free(struct reg *);
void         reg_bind(struct reg *, struct var *);
void         reg_set_type(struct reg *, type_t *);
void         reg_settypes(struct reg *, struct stack *);
struct reg * reg_copy(struct reg *);
void         reg_dump(const struct reg *);


struct cst {
	type_t type;
	union {
		int i;
		char c;
		double f;
		char *s;
		type_t o;
	};
};

struct cst * cst_new(type_t);
struct cst * cst_copy(struct cst *);
void         cst_free(void *);
void         cst_dump(const struct cst *);


#define E_CST 0
#define E_REG 1
#define E_VAR 2

struct elt {
	char elttype;
	union {
		struct reg *reg;
		struct cst *cst;
		struct var *var;
	};
};

struct elt * elt_new(char, void *);
void         elt_free(void *);
void *       elt_copy(void *element);
type_t       elt_type(const struct elt *);
void         elt_set_type(struct elt *, type_t);
void         elt_set_types(struct elt *, struct stack *);
void         elt_dump(const struct elt *);


struct function {
	char *fn;
	struct elt *ret;
	struct stack *params;
};

struct function * function_new(const char *);
void              function_free(void *);
void              function_dump(void *);


struct class {
	char *cn;
	type_t typenum;
	struct class *super;
	struct hashmap *attrs;
	struct hashmap *methods;
};

struct class * class_new(const char *name);
void           class_free(void *);
void           class_dump(void *);


#endif /* SYMTABLE_H */
