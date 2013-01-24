#include "stack.h"
#include "genfunc.h"
#include "instruction.h"
#include "types.h"
#include "gencode.h"

#include <stdlib.h>

static void func_gen_codes_rec(struct function *, struct stack *);
static void func_compute_var_type(struct function *, struct stack *);


/* Given a function with parameters not completely determined, this function
 * generates every possible function code to fit with every possible
 * parameters type.
 */
// TODO: add a hashmap parameter for local variables
void func_gen_codes(struct function *f, struct stack *instructions)
{
	// If some params type is UND_T then it should be put to
	// the list of every possible type
	stack_map(f->params, type_explicit, NULL, NULL);

	func_gen_codes_rec(f, instructions);
}

// TODO: add a hashmap parameter for local variables
void func_gen_codes_rec(struct function *f, struct stack *instructions)
{
	struct var *cur_param = NULL;
	struct type *cur_type = NULL;
	int i = 0;

	if (params_type_is_known(f)) {
		func_compute_var_type(f, instructions);
	}

	else {
		while ((cur_param = stack_peak(f->params, i++)) != NULL) {
			cur_type = stack_pop(cur_param->t);
			// If their was only one possible type, we cannot try to
			// generate a function code with a parameters without type
			if (stack_peak(cur_param->t, 0) != NULL) {
				func_gen_codes_rec(f, instructions);
			}
			stack_push(cur_param->t, cur_type);
		}
	}
}

/* Given a function with parameters type determined, this function
 * tries to determine the type for every local variable and return type.
 * If a type is found for each local variable then the function
 * code is generated.
 * Otherwise, no generation is performed.
 */
// TODO: add a hashmap parameter for local variables
void func_compute_var_type(struct function *f, struct stack *instructions)
{
	// TODO: use type_constrain on variable hashmap

	gencode_func(f, instructions);
}

