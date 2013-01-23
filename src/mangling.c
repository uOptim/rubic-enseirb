#include <stdlib.h>
#include <assert.h>
#include "stack.h"
#include "mangling.h"
#include "instruction.h"
#include "types.h"

/* Code generation */
static void func_gen_codes_rec(struct function *);
static void func_compute_var_type(struct function *);
static void func_mangling(struct function *);


/********************************************************************/
/*                          Code generation                         */
/********************************************************************/

/* Given a function with parameters not completely determined, this function
 * generates every possible function code to fit with every possible
 * parameters type.
 */
void func_gen_codes(struct function *f)
{
	// If some params type is UND_T then it should be put to
	// the list of every possible type
	stack_map(f->params, type_explicit, NULL);

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


