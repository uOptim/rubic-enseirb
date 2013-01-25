#include "stack.h"
#include "genfunc.h"
#include "instruction.h"
#include "types.h"
#include "gencode.h"

#include <stdlib.h>
#include <stdio.h>

static void    func_gen_codes_rec(struct function *, struct stack *, int);
static void    func_compute_var_type(struct function *, struct stack *);

static struct stack * instr_stack_copy(struct stack *);

#define DEBUG

#ifdef DEBUG
static void print_type(void *type, void *dummy1, void *dummy2) {
	if (dummy1 != NULL || dummy2 != NULL)
		return;

	type_t *t = (type_t *)type;
	printf("%d\t", *t);
}

static void print_types(void *variable, void *dummy1, void *dummy2) {
	struct var *v = (struct var *)variable;

	if (dummy1 != NULL || dummy2 != NULL)
		return;

	stack_map(v->t, print_type, NULL, NULL);
	printf("\n");
}
#endif /*DEBUG*/

/* Given a function with parameters not completely determined, this function
 * generates every possible function code to fit with every possible
 * parameters type.
 */
void func_gen_codes(struct function *f, struct stack *instructions)
{
#ifdef DEBUG
	//stack_map(f->params, print_types, NULL, NULL);
#endif /*DEBUG*/

	func_gen_codes_rec(f, instructions, 0);
}

void func_gen_codes_rec(struct function *f, struct stack *instructions, int i)
{
	struct var *cur_param = stack_peak(f->params, i);
	struct stack *tmp = stack_new();

	if (cur_param == NULL) {
		return;
	}

	// for each type of current parameter
	while (stack_peak(cur_param->t, 0) != NULL) {
		// generate current function code
		func_compute_var_type(f, instructions);
		// generate every combinaison for the following parameters
		func_gen_codes_rec(f, instructions, i+1);
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
void func_compute_var_type(struct function *f, struct stack *instructions)
{
	struct stack * i_copy = instr_stack_copy(instructions);

	//stack_map(i_copy, instr_constrain, NULL, NULL);

#ifdef DEBUG
	stack_map(f->params, print_types, NULL, NULL);
	printf("\n");
#else
	gencode_func(f, i_copy);
#endif /*DEBUG*/

	stack_free(&i_copy, instr_free);
}

struct stack * instr_stack_copy(struct stack * instructions)
{
	struct stack * s = stack_copy(instructions, instr_copy);

	return s;
}
