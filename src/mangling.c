#include <stdlib.h>
#include <assert.h>
#include "mangling.h"

struct instruction {
	char type;

	struct symbol *s1;
	struct symbol *s2; // might be unused for some instruction
};

/* Restrict a symbol type with a set of possible types given in an array
 *
 * s	is a symbol whose type may not be known
 * type is an array of possibles types for the symbol
 * n	is the size of this array
 */
void type_inter(struct symbol *s, const unsigned char types[], int n) {
	int i = 0;

	if (s->type != VAR_T) {
		return;
	}

	else {
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
}

/* Set possible symbol types according to the operation they appear in
 */
void type_constrain(int op_type, struct symbol *s1, struct symbol *s2) {
	unsigned char types[2] = {INT_T, FLO_T};
	if (op_type & I_ARI) {
		type_inter(s1, types, 2);
		type_inter(s2, types, 2);
	}
}

struct symbol * func_push_instr(struct function *f, int op_type,
		struct symbol *s1, struct symbol *s2) {
	struct instruction *i = malloc(sizeof *i);

	i->type = op_type;
	i->s1 = s1;
	i->s2 = s2;

	assert(f != NULL);

	stack_push(f->instr, i);

	return sym_new("OSEF", CST_T, cst_new(UND_T, CST_OPRESULT));
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

/* Given a fonction with parameters not completly determined, this function
 * generates every possible fonction code to fit with every possible
 * parameters type.
 */
void func_gen_codes(struct function *f) {

}
