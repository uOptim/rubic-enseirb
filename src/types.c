#include <stdlib.h>
#include "types.h"
#include "stack.h"

#define TYPE_NB	5
static type_t possible_types[TYPE_NB] = {INT_T, FLO_T, BOO_T,
	STR_T, OBJ_T};

static void var_type_is_known(void *variable, void *params);

/********************************************************************/
/*                        Type computation                          */
/********************************************************************/

/* Restrict a variable type with a set of possible types given in an array
 *
 * s	is a variable whose type may not be known
 * type is an array of possibles types for the symbol
 * n	is the size of this array
 */
void type_inter(struct var *v, const type_t types[], int n)
{
	int i = 0;
	struct stack *stmp = stack_new();
	type_t * cur_type = NULL;

	// the symbol type was undefined
	// we have now some idea of its possible types
	if (var_gettype(v) == UND_T) {
		free(stack_pop(v->t));

		for (i = 0; i < n; i++) {
			var_pushtype(v, types[i]);
		}
	}

	// we keep possible types of the symbol that are in the given
	// set of types
	else {
		while ((cur_type = stack_pop(v->t)) != NULL) {
			for (i = 0; i < n; i++) {
				if (*cur_type == types[i]) {
					stack_push(stmp, cur_type);
					break;
				}
			}

			if (i == n) {
				// This type isn't possible anymore
				free(cur_type);
			}
		}

		// now stmp contains the new possible types for v
		// let's update the actual symbol possible types
		stack_move(stmp, v->t);
	}

	stack_free(&stmp, NULL);
}


/********************************************************************/
/*                         Type operations                          */
/********************************************************************/

/* Returns 1 if every parameters type is determined for f
 * and 0 otherwise.
 * A type is determined when there is only one possible type
 * and it is not UND_T.
 */
int params_type_is_known(struct function *f)
{
	int is_known = 1;

	stack_map(f->params, var_type_is_known, &is_known);

	return is_known;
}

void var_type_is_known(void *variable, void *params) {
	struct var *v = (struct var *)variable;
	int *is_known = (int *)params;

	if (var_gettype(v) == UND_T || var_type_card(v) != 1) {
		*is_known = 0;
	}
}

/* If the variable type is UND_T then it is changed to the list of every
 * possible type.
 * Otherwise no change is performed.
 */
void type_explicit(void *variable, void *params)
{
	int i = 0;
	struct var *v = (struct var *)variable;

	if (params != NULL) {
		return;
	}

	if (*((type_t *)stack_pop(v->t)) == UND_T) {
		for (; i < TYPE_NB; i++) {
			var_pushtype(v, possible_types[i]);
		}
	}
}
