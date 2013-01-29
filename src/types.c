#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "stack.h"

type_t possible_types[TYPE_NB] = {INT_T, FLO_T, BOO_T, STR_T};


/********************************************************************/
/*                         Type operations                          */
/********************************************************************/

void type_init(struct stack *types) {
	int i = 0;
	for (; i < TYPE_NB; i++) {
		stack_push(types, &possible_types[i]);
	}
}

type_t type_new(void) {
	static type_t id = TYPE_NB;

	return id++;
}

void var_put_types(struct var *v, struct stack *types) {
	stack_free(&v->t, NULL);
	v->t = stack_copy(types, type_copy);
}

void var_set_type(struct var *v, type_t *type) {
	stack_clear(v->t, NULL);
	stack_push(v->t, type);
}

void * type_copy(void *type) {
	return type;
}


/********************************************************************/
/*                        Type computation                          */
/********************************************************************/

int type_ispresent(struct stack *types, type_t type)
{
	type_t *t;

	stack_rewind(types);
	while ((t = (type_t *) stack_next(types)) != NULL) {
		if (*t == type) {
			stack_rewind(types);
			return 1;
		}
	}

	return 0;
}

/* Returns the intersection of two sets of types given in stacks.
 * A stack containing the intersection is returned. */
struct stack * type_inter(struct stack *t1, struct stack *t2)
{
	type_t *t, *u;
	struct stack *res = stack_new();

	/* DEBUG STUFF
	stack_rewind(t1);
	stack_rewind(t2);
	fprintf(stderr, "Intersectig types: ");
	while ((t = (type_t *) stack_next(t1)) != NULL) {
		fprintf(stderr, "%d ", *t);
	}
	fprintf(stderr, "\nWith types: ");
	while ((t = (type_t *) stack_next(t2)) != NULL) {
		fprintf(stderr, "%d ", *t);
	}
	*/

	stack_rewind(t1);
	stack_rewind(t2);
	while ((t = (type_t *) stack_next(t1)) != NULL) {
		while ((u = (type_t *) stack_next(t2)) != NULL) {
			if (*u == *t) {
				stack_push(res, t);
			}
		}
	}

	/* DEBUG STUFF
	stack_rewind(res);
	fprintf(stderr, "\nResult: ");
	while ((t = (type_t *) stack_next(res)) != NULL) {
		fprintf(stderr, "%d ", *t);
	}
	fprintf(stderr, "\n");
	*/

	return res;
}


struct stack * type_union(struct stack *t1, struct stack *t2)
{
	type_t *t;
	struct stack *u = stack_new();

	stack_rewind(t1);
	while ((t = (type_t *) stack_next(t1)) != NULL) {
		stack_push(u, t);
	}

	stack_rewind(t2);
	while ((t = (type_t *) stack_next(t2)) != NULL) {
		if (!type_ispresent(u, *t)) {
			stack_push(u, t);
		}
	}

	return u;
}
