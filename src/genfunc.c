#include "genfunc.h"
#include "instruction.h"
#include "types.h"
#include "gencode.h"

#include <stdlib.h>
#include <stdio.h>

static void func_gen_codes_rec(struct function *, struct stack *, 
		struct hashmap *, int);

static void func_compute_var_type(struct function *, struct stack *,
		struct hashmap *);

static struct stack * instr_stack_copy(struct stack *);

/* Given a function with parameters not completely determined, this function
 * generates every possible function code to fit with every possible
 * parameters type.
 */
void func_gen_codes(
		struct function *f,
		struct stack *instructions, 
		struct hashmap * h)
{
	// function without parameter
	if (stack_peak(f->params, 0) == NULL) {
		gencode_func(f, f->fn, instructions, h);
	}
	// function with parameters, generate every combinaison
	else {
		func_gen_codes_rec(f, instructions, h, 0);
	}
}

void func_gen_codes_rec(
		struct function *f,
	    struct stack *instructions, 
		struct hashmap * h,
		int i)
{
	struct var *cur_param = stack_peak(f->params, i);

	if (cur_param == NULL) {
		return;
	}

	struct stack *tmp = stack_new();

	// for each type of current parameter
	while (stack_peak(cur_param->t, 0) != NULL) {
		// generate current function code
		func_compute_var_type(f, instructions, h);
		// generate every combinaison for the following parameters
		func_gen_codes_rec(f, instructions, h, i+1);
		stack_push(tmp, stack_pop(cur_param->t));
	}
	stack_move(tmp, cur_param->t);
	stack_free(&tmp, NULL);
}

/* Given a function with parameters type determined, this function
 * tries to determine the type for every local variable and return type.
 * If a type is found for each local variable then the function
 * code is generated.
 * Otherwise, no generation is performed.
 */
void func_compute_var_type(
		struct function *f,
		struct stack *instructions,
		struct hashmap * h)
{
	struct stack * i_copy = instr_stack_copy(instructions);
	const char * fnm = func_mangling(f);

	//stack_map(i_copy, instr_constrain, NULL, NULL);

	if (hashmap_get(h, fnm) == NULL) {
		hashmap_set(h, fnm, DUMMY_FUNC);
		gencode_func(f, fnm, i_copy, h);
	}
	else {
		free(fnm);
	}

	stack_free(&i_copy, instr_free);
}

struct stack * instr_stack_copy(struct stack * instructions)
{
	struct stack * s = stack_copy(instructions, instr_copy);

	return s;
}
