#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "stack.h"

/* Type */
#define FUN_T   0
#define CLA_T   1
#define OBJ_T   2

#define INT_T   3
#define FLO_T   4
#define STR_T   5
#define BOO_T   6

#define UND_T   127 // Undef variable


struct var {
	char *vn;

	// type
	union {
		unsigned char t;         // general type
		struct {
			unsigned char tt:7;  // specific type
			unsigned char tc:1;  // is const
		};
	};

	// value
	union {
		int    in;
		char   bo;
		char * st;
		float  fl;

		struct object {
			char *cn;
			struct stack *attrs;
		} ob;
	};
};


struct function {
	char *fn; 
	struct var *ret;
	struct stack *params;
};


struct class {
	char *cn;
	struct class *super;
	struct stack *methods;
};


struct var * var_new(const char *);
void         var_set(struct var *, int, void *);
void         var_free(void *);
void         var_dump(void *);

struct class * class_new(const char *, struct class *);
void           class_free(void *);
void           class_dump(void *);

struct function * function_new();
void              function_free(void *);
void              function_dump(void *);

#endif /* SYMTABLE_H */
