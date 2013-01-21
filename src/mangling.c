#include <stdlib.h>
#include <assert.h>
#include "mangling.h"

#define TYPE_NB	5
static unsigned char possible_types[TYPE_NB] = {INT_T, FLO_T, BOO_T,
	STR_T, OBJ_T};

static int sym_t_nb(struct symbol *);
static int sym_type(struct symbol *);

struct instruction {
	char op_type;

	struct symbol *sr; // returned symbol

	struct symbol *s1;
	struct symbol *s2; // might be unused for some instruction
};

/* Restrict a symbol type with a set of possible types given in an array
 *
 * s	is a symbol whose type may not be known
 * type is an array of possibles types for the symbol
 * n	is the size of this array
 */
static void type_inter(struct symbol *s, const unsigned char types[], int n) {
	int i = 0;

	if (s->type != VAR_T) {
		return;
	}

	struct stack *sym_types = ((struct var *)s->ptr)->t;
	struct stack *stmp = stack_new();
	struct type *cur_type = stack_pop(sym_types);
	assert(cur_type != NULL);

	// the symbol type was undefined
	// we have now some idea of its possible types
	if (cur_type->tt == UND_T) {
		for (i = 0; i < n; i++) {
			stack_push(sym_types, type_new(types[i]));
		}
		free(cur_type);
	}

	// we keep possible types of the symbol that are in the given
	// set of types
	else {
		do {
			for (i = 0; i < n; i++) {
				if (cur_type->tt == types[i]) {
					stack_push(stmp, cur_type);
					break;
				}
			}

			if (i == n) {
				// This type isn't possible anymore
				free(cur_type);
			}

		} while ((cur_type = stack_pop(sym_types)) != NULL);

		// now stmp contains the new possible types for s
		// let's update the actual symbol possible types
		while ((cur_type = stack_pop(stmp)) != NULL) {
			stack_push(sym_types, cur_type);
		}
	}

	stack_free(&stmp, NULL);
}

/* Set possible symbol types according to the operation they appear in
*/
static void type_constrain(struct instruction *i) {
	unsigned char types[2] = {INT_T, FLO_T};

	if (i->op_type & I_ARI) {
		type_inter(i->s1, types, 2);
		type_inter(i->s2, types, 2);
		if (sym_t_nb(i->s1) == 1 && sym_t_nb(i->s2) == 1) {
			types[0] = compatibility_table[sym_type(i->s1)][sym_type(i->s1)];
			type_inter(i->sr, types, 1);
		}
		else {
			type_inter(i->sr, types, 2);
		}
	}
}

struct symbol * func_push_instr(struct function *f, int op_type,
		struct symbol *s1, struct symbol *s2) {
	struct instruction *i = malloc(sizeof *i);

	i->op_type = op_type;
	i->sr = sym_new("OSEF", CST_T, cst_new(UND_T, CST_OPRESULT));
	i->s1 = s1;
	i->s2 = s2;
	type_constrain(i);

	assert(f != NULL);

	stack_push(f->instr, i);

	return i->sr;
}

/* Given a fonction with parameters and local variables type determined,
 * the function code is generated.
 * The code is printed on stdout.
 */
static void func_mangling(struct function *f) {

}

/* Given a fonction with parameters type determined, this function
 * tries to determine the type for every local variable.
 * If a type is found for each local variable then the function
 * code is generated.
 * Otherwise, no generation is performed.
 */
static void func_compute_var_type(struct function *f) {

}

int func_param_type_det(struct function *);

static void func_gen_codes_rec(struct function *f) {
	struct var *cur_param = NULL;
	struct type *cur_type = NULL;
	int i = 0;

	if (func_param_type_det(f)) {
		func_compute_var_type(f);
	}

	else {
		while ((cur_param = stack_peak(f->params, i++)) != NULL) {
			cur_type = stack_pop(cur_param->t);
			// If their was only one possible type, we cannot try to
			// generate a function code with a parameters without type
			if (stack_peak(cur_param->t, 0) != NULL) {
				func_gen_codes_rec(f);
			}
			stack_push(cur_param->t, cur_type);
		}
	}
}

void type_explicit(struct stack *);

/* Given a fonction with parameters not completly determined, this function
 * generates every possible fonction code to fit with every possible
 * parameters type.
 */
void func_gen_codes(struct function *f) {
	struct stack *tmp = stack_new();
	struct var *cur_param = NULL;

	// If some params type is UND_T then it should be put to
	// the list of every possible type
	while ((cur_param = stack_pop(f->params)) != NULL) {
		type_explicit(cur_param->t);
		stack_push(tmp, cur_param);
	}
	// Recover the previous stack
	while ((cur_param = stack_pop(tmp)) != NULL) {
		stack_push(f->params, cur_param);
	}
	stack_free(&tmp, NULL);

	func_gen_codes_rec(f);
}

/* Returns 1 if every parameters type is determined for f
 * and 0 otherwise.
 * A type is determined when there is only one possible type
 * and it is not UND_T.
 */
int func_param_type_det(struct function *f) {
	struct stack * tmp = stack_new();
	struct var * cur_var = NULL;

	while ((cur_var = stack_pop(f->params)) != NULL) {
		if (((struct type *)stack_peak(cur_var->t, 0))->tt == UND_T
				|| stack_peak(cur_var->t, 1) != NULL) {
			return 0;
		}
	}
	return 1;
}

/* Given a type stack pointer representing a set of possible type,
 * if type is UND_T then it is changed to the list of every possible type.
 * Otherwise no change is performed.
 */
void type_explicit(struct stack *t) {
	int i = 0;

	if (((struct type *)stack_pop(t))->tt == UND_T) {
		for (; i < TYPE_NB; i++) {
			stack_push(t, type_new(possible_types[i]));
		}
	}
}

/* Given a symbole representing a variable, returns the number of types
 * possible for this variable
 */
static int sym_t_nb(struct symbol *s) {
	return stack_size(((struct var *)s->ptr)->t);
}

/* Given a symbole representing a variable, returns the first type possible
 * for this variable
 */
static unsigned char sym_type(struct symbol *s) {
	return ((struct type *)stack_peak(((struct var *)s->ptr)->t, 0))->tt;
}
