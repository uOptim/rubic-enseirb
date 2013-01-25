#include <stdlib.h>
#include "types.h"
#include "stack.h"

#define TYPE_NB	4
static type_t possible_types[TYPE_NB] = {INT_T, FLO_T, BOO_T, STR_T};

static void var_type_is_known(void *variable, void *params, void* dummy);
static void * type_copy(void *);


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

void var_put_types(struct var *v, const struct stack *types) {
	stack_free(&v->t, free);

	v->t = stack_copy(types, type_copy);
}

static void * type_copy(void *type) {
	type_t * t = malloc(sizeof *t);

	*t = *(type_t *)type;

	return (void *)t;
}

/* Returns 1 if every parameters type is determined for f
 * and 0 otherwise.
 * A type is determined when there is only one possible type
 * and it is not UND_T.
 */
int params_type_is_known(struct function *f)
{
	int is_known = 1;

	stack_map(f->params, var_type_is_known, &is_known, NULL);

	return is_known;
}

void var_type_is_known(void *variable, void *params, void* dummy) {
	struct var *v = (struct var *)variable;
	int *is_known = (int *)params;

	if (dummy != NULL) {
		return;
	}

	if (var_gettype(v) == UND_T || var_type_card(v) != 1) {
		*is_known = 0;
	}
}

/********************************************************************/
/*                        Type computation                          */
/********************************************************************/

/* Restrict a variable type with a set of possible types given in an array
 *
 * v	 is a variable whose type may not be known
 * types is an array of possibles types for the variable
 * n	 is the size of this array
 */
struct stack * type_inter(struct stack *t1, struct stack *t2)
{
	type_t t, u;
	struct stack *res = stack_new();

	stack_rewind(t1);
	stack_rewind(t2);
	while ((t = *(type_t *) stack_next(t1)) != NULL) {
		while ((u = *(type_t *) stack_next(t2)) != NULL) {
			if (u == t) {
				stack_push(res, v);
			}
		}
	}

	return res;
}
