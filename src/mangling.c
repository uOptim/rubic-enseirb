#include <stdlib.h>
#include <assert.h>
#include "stack.h"
#include "mangling.h"

#define TYPE_NB	5
static unsigned char possible_types[TYPE_NB] = {INT_T, FLO_T, BOO_T,
	STR_T, OBJ_T};

struct instruction {
	char op_type;

	struct symbol *sr; // returned symbol

	struct symbol *s1;
	struct symbol *s2; // might be unused for some instruction
};

/* Instruction operations */
static struct instruction * instr_new(int, struct symbol *, struct symbol *);

/* Code generation */
static void func_gen_codes_rec(struct function *);
static void func_compute_var_type(struct function *);
static void func_mangling(struct function *);

/* Type computation */
static void type_constrain(struct instruction *);
static void type_inter(struct symbol *, const unsigned char types[], int);

/* Type operations */
static int            params_type_is_known(struct function *);
static void           type_explicit(struct var *);
static int            var_type_card(struct var *);
static unsigned char  var_type(struct var *);


/********************************************************************/
/*                      Instruction operations                      */
/********************************************************************/

struct instruction * instr_new(
	int op_type,
	struct symbol *s1,
	struct symbol *s2)
{
	struct instruction *i = malloc(sizeof *i);

	i->op_type = op_type;
	i->sr = sym_new("OSEF", CST_T, cst_new(UND_T, CST_OPRESULT));
	i->s1 = s1;
	i->s2 = s2;
	type_constrain(i);

	return i;
}

struct symbol * instr_push(
	struct function *f,
	int op_type,
	struct symbol *s1,
	struct symbol *s2)
{

	struct instruction * i = instr_new(op_type, s1, s2);
	assert(f != NULL);
	stack_push(f->instr, i);

	return i->sr;
}

void instr_print(int op_type, struct symbol *s1, struct symbol *s2)
{
	struct instruction * i = instr_new(op_type, s1, s2);

	// TODO print instruction code
}

/********************************************************************/
/*                          Code generation                         */
/********************************************************************/

/* Given a function with parameters not completely determined, this function
 * generates every possible function code to fit with every possible
 * parameters type.
 */
void func_gen_codes(struct function *f)
{
	struct stack *tmp = stack_new();
	struct var *cur_param = NULL;

	// If some params type is UND_T then it should be put to
	// the list of every possible type
	while ((cur_param = stack_pop(f->params)) != NULL) {
		type_explicit(cur_param);
		stack_push(tmp, cur_param);
	}
	// Recover the previous stack
	stack_move(tmp, f->params);
	stack_free(&tmp, NULL);

	func_gen_codes_rec(f);
}

void func_gen_codes_rec(struct function *f)
{
	struct var *cur_param = NULL;
	struct type *cur_type = NULL;
	int i = 0;

	if (params_type_is_known(f)) {
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

/* Given a function with parameters type determined, this function
 * tries to determine the type for every local variable.
 * If a type is found for each local variable then the function
 * code is generated.
 * Otherwise, no generation is performed.
 */
void func_compute_var_type(struct function *f)
{
	// TODO: use type_constrain

	func_mangling(f);
}

/* Given a function with parameters and local variables type determined,
 * the function code is generated.
 * The code is printed on stdout.
 */
void func_mangling(struct function *f) {
	// TODO: use instr_print
}


/********************************************************************/
/*                        Type computation                          */
/********************************************************************/

/* Set possible symbol types according to the operation they appear in
 */
void type_constrain(struct instruction *i)
{
	unsigned char types[2] = {INT_T, FLO_T};

	if (i->op_type & I_ARI) {
		type_inter(i->s1, types, 2);
		type_inter(i->s2, types, 2);
		if (var_type_card((struct var*)i->s1->ptr) == 1
				&& var_type_card((struct var*)i->s2->ptr) == 1) {
			// Common type for variables stored in s1 and s2
			types[0] = compatibility_table
				[var_type((struct var *)i->s1->ptr)]
				[var_type((struct var *)i->s2->ptr)];
			type_inter(i->sr, types, 1);
		}
		else {
			type_inter(i->sr, types, 2);
		}
	}
}

/* Restrict a symbol type with a set of possible types given in an array
 *
 * s	is a symbol whose type may not be known
 * type is an array of possibles types for the symbol
 * n	is the size of this array
 */
void type_inter(struct symbol *s, const unsigned char types[], int n)
{
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
		stack_move(stmp, sym_types);
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
	struct stack * tmp = stack_new();
	struct var * cur_var = NULL;
	int is_known = 1;

	while ((cur_var = stack_pop(f->params)) != NULL) {
		stack_push(tmp, cur_var);
		if (var_type(cur_var) == UND_T || var_type_card(cur_var) != 1) {
			is_known = 0;
		}
	}
	stack_move(tmp, f->params);
	stack_free(&tmp, NULL);

	return is_known;
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

	if (((struct type *)stack_pop(v->t))->tt == UND_T) {
		for (; i < TYPE_NB; i++) {
			stack_push(v->t, type_new(possible_types[i]));
		}
	}
}

/* Returns the number of types possible for a variable
 */
int var_type_card(struct var *v)
{
	return stack_size(v->t);
}

/* Returns the first possible variable type
 */
unsigned char var_type(struct var *v)
{
	return ((struct type *)stack_peak(v->t, 0))->tt;
}
