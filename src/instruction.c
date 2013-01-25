#include "instruction.h"
#include "gencode.h"
#include "types.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>



static struct instr * instr_new(
		int optype,
		struct var * vr,
		struct elt * er,
		struct elt * e1,
		struct elt * e2)
{
	if (optype == I_RAW) {
		return NULL;
	}

	struct instr *i = malloc(sizeof *i);

	i->vr = vr;
	i->er = er;
	i->e1 = e1;
	i->e2 = e2;
	i->optype = optype;

	return i;
}


void * instr_copy(void * instruction) {
	struct instr * i = (struct instr *)instruction;

	return instr_new(i->optype,
			var_copy(i->vr),
			elt_copy(i->er),
			elt_copy(i->e1),
			elt_copy(i->e2)
		);
}


void instr_free(void *instruction)
{
	struct instr *i = (struct instr *) instruction;

	if (i->optype == I_RAW) {
		free(i->rawllvm);
		i->rawllvm = NULL;
	}

	else {
		// do not free vr it is used in the global hashmap.
		if (i->er != NULL) {
			elt_free(i->er);
			i->er = NULL;
		}

		if (i->e1 != NULL) {
			elt_free(i->e1);
			i->e1 = NULL;
		}
		
		if (i->e2 != NULL) {
			elt_free(i->e2);
			i->e2 = NULL;
		}
	}

	free(i);
}

static int type_vartype_constrain_ari(struct elt *e)
{
	int ret = 0;
	struct stack *tmp, *inter;

	// TODO: Init this only once at the begining
	tmp = stack_new();
	stack_push(tmp, &possible_types[FLO_T]);
	stack_push(tmp, &possible_types[INT_T]);

	if (e->elttype == E_REG) {
		inter = type_inter(tmp, e->reg->types);
		if (stack_size(inter) == 0) {
			fprintf(stderr, "Invalid type for arithmetic operation");
		}
		
		else {
			// replace old stack with the new one
			reg_settypes(e->reg, inter);
			ret = stack_size(inter);
		}

		stack_free(&inter, NULL);
	} 
	
	else {
		if (e->cst->type != FLO_T && e->cst->type != INT_T) {
			fprintf(stderr, "Invalid type for arithmetic operation");
		} else {
			ret = 1;
		}
	}

	stack_free(&tmp, NULL);

	return ret;
}

struct stack * type_constrain_ari(struct elt *e1, struct elt *e2)
{
	struct stack *types, *tmp1, *tmp2;

	// these will modify the type of e1 and e2 to match arithmetic operations
	// if possible.
	if (0 == type_vartype_constrain_ari(e1)) { return NULL; }
	if (0 == type_vartype_constrain_ari(e2)) { return NULL; }

	if (e1->elttype == E_REG) {
		tmp1 = e1->reg->types;
	} else {
		tmp1 = stack_new();
		stack_push(tmp1, &possible_types[(int)e1->cst->type]);
	}

	if (e2->elttype == E_REG) {
		tmp2 = e2->reg->types;
	} else {
		tmp2 = stack_new();
		stack_push(tmp2, &possible_types[(int)e2->cst->type]);
	}

	types = type_inter(tmp1, tmp2);

	if (e1->elttype == E_CST) { stack_free(&tmp1, NULL); }
	if (e2->elttype == E_CST) { stack_free(&tmp2, NULL); }

	return types;
}

struct instr * i3addr(char optype, struct elt *e1, struct elt *e2)
{
	struct reg *reg;
	struct instr *i;
	struct stack *types;

	if (optype & I_ARI) {
		types = type_constrain_ari(e1, e2);
		if (types == NULL) { return NULL; }
	}
	
	else if (optype & I_CMP) {
		;
	}

	else if (optype & I_BOO) {
		;
	}
	
	else {
		fprintf(stderr, "Unrecognized types.\n");
		return NULL;
	}

	reg = reg_new(NULL);
	reg_settypes(reg, types);
	i = instr_new(optype, NULL, elt_new(E_REG, reg), e1, e2);

	stack_free(&types, NULL);
	return i;
}

struct instr * iraw(const char *s)
{
	struct instr *i = malloc(sizeof *i);

	if (i == NULL) {
		perror("malloc");
		return NULL;
	}

	i->optype = I_RAW;
	i->rawllvm = strdup(s);

	if (i->rawllvm == NULL) {
		perror("strdup");
		free(i);
		i = NULL;
	}

	return i;
}


struct instr * iret(struct elt *elt)
{
	struct instr *i;
	i = instr_new(I_RET, NULL, elt, NULL, NULL);

	return i;
}

struct instr * ialloca(struct var *vr)
{
	struct instr *i;
	i = instr_new(I_ALO, vr, NULL, NULL, NULL);

	return i;
}

struct instr * iload(struct var *vr)
{
	struct instr *i;
	struct reg *reg = reg_new(vr);

	i = instr_new(
			I_LOA, 
			vr,
			elt_new(E_REG, reg),
			NULL,
			NULL
		);

	return i;
}

struct instr * istore(struct var *vr, struct elt *elt)
{
	struct instr *i;
	struct stack *typeinter;

	if (elt->elttype == E_REG) {
		typeinter = type_inter(vr->t, elt->reg->types);

		if (stack_size(typeinter) == 0) {
			fprintf(stderr, "Error: Incompatible types in assignment\n");
			return NULL;
		}

		stack_free(&vr->t, NULL);
		vr->t = typeinter;
		reg_bind(elt->reg, vr);

		i = instr_new(I_STO, vr, elt, NULL, NULL);
	}

	else {
		fprintf(stderr, "Impossible error :)\n");
		return NULL;
	}

	return i;
}


struct elt * instr_get_result(const struct instr * i)
{
	return elt_copy(i->er);
}
