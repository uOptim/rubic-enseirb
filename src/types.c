#include <stdlib.h>
#include "types.h"
#include "stack.h"

type_t possible_types[TYPE_NB] = {INT_T, FLO_T, BOO_T, STR_T};

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

void var_put_types(struct var *v, struct stack *types) {
	stack_free(&v->t, NULL);
	v->t = stack_copy(types, type_copy);
}

static void * type_copy(void *type) {
	return type;
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

/* Returns the intersection of two sets of types given in stacks.
 * A stack containing the intersection is returned.
 */
struct stack * type_inter(struct stack *t1, struct stack *t2)
{
	type_t *t, *u;
	struct stack *res = stack_new();

	stack_rewind(t1);
	stack_rewind(t2);
	while ((t = (type_t *) stack_next(t1)) != NULL) {
		while ((u = (type_t *) stack_next(t2)) != NULL) {
			if (*u == *t) {
				stack_push(res, t);
			}
		}
	}

	return res;
}

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

