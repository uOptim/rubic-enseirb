#include "instruction.h"
#include "gencode.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static struct symbol * sym_new(char, void *);
static void            sym_free(struct symbol **);



static struct instr* instr_new(
		int op_type,
		struct var * vr,
		struct res * res,
		struct cst * c1,
		struct cst * c2)
{
	if (op_type == I_RAW) {
		return NULL;
	}

	struct instr *i = malloc(sizeof *i);

	i->vr = vr;
	i->res = res;
	i->c1 = c1;
	i->c2 = c2;
	i->op_type = op_type;

	return i;
}

void * instr_copy(void * instruction) {
	struct instr * i = (struct instr *)instruction;

	return (void *)instr_new(
			i->op_type,
			i->vr,
			i->cr,
			i->c1,
			i->c2
			);
}

void instr_free(void *instruction)
{
	struct instr *i = (struct instr *) instruction;

	if (i->op_type == I_RAW) {
		free(i->rawllvm);
		i->rawllvm = NULL;
	}

	else {
		// do not free symbols of type VAR_T, they are used elsewhere.
		if (i->res != NULL) {
			res_free(i->res);
			i->res = NULL;
		}

		if (i->c1 != NULL) {
			cst_free(i->c1);
			i->c1 = NULL;
		}
		
		if (i->c2 != NULL) {
			cst_free(i->c2);
			i->c2 = NULL;
		}
	}

	free(i);
}

/* Set possible symbol types according to the operation they appear in
*/
void instr_constrain(void *instruction, void *dummy1, void *dummy2)
{
	struct instr *i = (struct instr *)instruction;
	if (dummy1 != NULL || dummy2 != NULL) {
		return;
	}
	/*
	if (i->op_type & I_ARI) {
		type_t types[2] = {INT_T, FLO_T};

		type_inter(i->c1->var, types, 2);
		type_inter(i->c2->var, types, 2);
		if (var_type_card((struct var*)i->c1->var) == 1
				&& var_type_card((struct var*)i->c2->var) == 1) {
			// Common type for variables stored in c1 and c2
			types[0] = compatibility_table
				[var_gettype((struct var *)i->c1->var)]
				[var_gettype((struct var *)i->c2->var)];
			type_inter(i->cr->var, types, 1);
		}
		else {
			type_inter(i->cr->var, types, 2);
		}
	}
	*/
}

struct instr * iraw(const char *s)
{
	struct instr *i = malloc(sizeof *i);

	if (i == NULL) {
		perror("malloc");
		return NULL;
	}

	i->op_type = I_RAW;
	i->rawllvm = strdup(s);

	if (i->rawllvm == NULL) {
		perror("strdup");
		free(i);
		return NULL;
	}

	return i;
}


struct instr * i3addr(char optype, struct cst *c1, struct cst *c2)
{
	struct res *res;
	struct instr *i;


	if (c1->type == UND_T || c2->type == UND_T) {
		// we are in a function
	}

	else if (optype & I_ARI) {
		res = res_new(NULL);
		if (   (c1->type != FLO_T && c1->type != INT_T)
			|| (c2->type != FLO_T && c2->type != INT_T)) {
			fprintf(stderr, "Incompatible types for arithmetic operation.\n");
			return NULL;
		}

		res_pushtype(res, FLO_T);
		if (c1->type == INT_T || c2->types == INT_T) {
			res_pushtype(res, INT_T);
		}
	} 
	
	else if (optype & I_CMP) {
		res = res_new(NULL);
		if (   (c1->type != FLO_T && c1->type != INT_T)
			|| (c2->type != FLO_T && c2->type != INT_T)) {
			fprintf(stderr, "Incompatible types for comparison operation.\n");
			return NULL;
		}

		res_pushtype(res, BOO_T);
	}

	else if (optype & I_BOO) {
		res = res_new(NULL);
		if (c1->type != BOO_T || c2->type != BOO_T) {
			fprintf(stderr, "Incompatible types for boolean operation.\n");
			return NULL;
		}

		res_pushtype(res, BOO_T);
	}
	
	else {
		fprintf(stderr, "Unrecognized types.\n");
		return NULL;
	}

	i = instr_new(optype, NULL, res, c1, c2);

	return i;
}

struct instr * iret(const struct res *res)
{
	struct instr *i;
	i = instr_new(I_RET, NULL, res, NULL, NULL);

	return i;
}

struct instr * ialloca(const struct var *vr)
{
	struct instr *i;
	i = instr_new(I_ALO, vr, res_new(vr), NULL, NULL);

	return i;
}

struct instr * iload(struct var *vr)
{
	struct instr *i;

	i = instr_new(
			I_LOA, 
			vr,
			res_new(vr),
			NULL,
			NULL
		);

	return i;
}

struct instr * istore(struct var *vr, struct res *res)
{
	struct instr *i;
	struct stack *typeinter;

	typeinter = type_inter(vr->t, res->types);

	if (stack_size(tmp) == 0) {
		fprintf(stderr, "Error: Incompatible types in assignment\n");
		return NULL;
	}

	stack_free(&vr->t, NULL);
	vr->t = typeinter;
	res_bind(vr);

	i = instr_new(I_STO, vr, res, NULL, NULL);

	return i;
}


struct cst * instr_get_result(const struct instr * i)
{
	return res_copy(i->res);
}
