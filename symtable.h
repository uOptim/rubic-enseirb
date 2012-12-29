#include "stack.h"

/* Type */
#define FUN_T   0
#define CLA_T   1
#define OBJ_T   2

#define INT_T   3
#define FLO_T   4
#define STR_T   5


struct type {
	union {
		unsigned char t;         // general type
		struct {
			unsigned char tt:7;  // specific type
			unsigned char tc:1;  // is const
		};
	};

	union {
		int    in;
		char   by;
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
	struct type *ret;
	struct stack *params;
};


struct class {
	char *cn;
	struct class *super;
	struct stack *methods;
};


struct type * type_new(int, void *);
void          type_free(void *);
void          type_dump(void *);

struct class * class_new(const char *, struct class *);
void           class_free(void *);
void           class_dump(void *);

struct function * function_new(const char *, struct stack *);
void              function_free(void *);
void              function_dump(void *);
