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
	if (optype == I_RAW || optype == I_CAST || optype == I_CAL) {
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


void * instr_copy(void * instruction)
{
	struct instr *copy;
	struct instr * i = (struct instr *)instruction;

	if (i->optype == I_RAW) { 
		copy = malloc(sizeof *copy);
		copy->rawllvm = strdup(i->rawllvm);
	}

	else if (i->optype == I_CAST) {
		copy = malloc(sizeof *copy);
		copy->cast_func = i->cast_func;
		copy->tocast = elt_copy(i->tocast);
		copy->res = elt_copy(i->res);
	}

	else if (i->optype == I_CAL) {
		copy = malloc(sizeof *copy);
		copy->fn = strdup(i->fn);
		copy->args = stack_copy(i->args, elt_copy);
		copy->ret = elt_copy(i->ret);
	}

	else {
		copy = instr_new(i->optype,
			i->vr,
			elt_copy(i->er),
			elt_copy(i->e1),
			elt_copy(i->e2)
		);
	}

	return copy;
}


void instr_free(void *instruction)
{
	struct instr *i = (struct instr *) instruction;

	if (i->optype == I_RAW) { free(i->rawllvm); }

	else if (i->optype == I_CAST) {
		elt_free(i->res);
		elt_free(i->tocast);
	}

	else if (i->optype == I_CAL) {
		free(i->fn);
		stack_free(&i->args, elt_free);
		stack_free(&i->ret->reg->types, NULL);
		elt_free(i->ret);
	}

	else {
		// do not free vr it is used in the global hashmap.
		if (i->er != NULL) { elt_free(i->er); }
		if (i->e1 != NULL) { elt_free(i->e1); }
		if (i->e2 != NULL) { elt_free(i->e2); }
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
		ret = stack_size(inter);

		if (ret != 0) {
			// replace old stack with the new one
			reg_settypes(e->reg, inter);
		}

		stack_free(&inter, NULL);
	} 
	
	else if (e->elttype == E_VAR) {
		inter = type_inter(tmp, e->var->t);
		ret = stack_size(inter);

		if (ret != 0) {
			// replace old stack with the new one
			var_put_types(e->var, inter);
		}

		stack_free(&inter, NULL);
	} 

	else {
		if (e->cst->type != FLO_T && e->cst->type != INT_T) {
			ret = 0;
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

	// these will modify the type of e1 and e2 to match arithmetic operation.
	// if possible.
	if (type_vartype_constrain_ari(e1) == 0) { return NULL; }
	if (type_vartype_constrain_ari(e2) == 0) { return NULL; }

	if (e1->elttype == E_REG) {
		tmp1 = e1->reg->types;
	}
	else if (e1->elttype == E_VAR) {
		tmp1 = e1->var->t;
	} else {
		tmp1 = stack_new();
		stack_push(tmp1, &possible_types[(int)e1->cst->type]);
	}

	if (e2->elttype == E_REG) {
		tmp2 = e2->reg->types;
	}
	else if (e2->elttype == E_VAR) {
		tmp2 = e2->var->t;
	} else {
		tmp2 = stack_new();
		stack_push(tmp2, &possible_types[(int)e2->cst->type]);
	}

	types = type_inter(tmp1, tmp2);
	if (stack_size(types) == 0) { // float + int
		stack_push(types, &possible_types[FLO_T]);
	}

	if (e1->elttype == E_CST) { stack_free(&tmp1, NULL); }
	if (e2->elttype == E_CST) { stack_free(&tmp2, NULL); }

	return types;
}

static int type_vartype_constrain_cmp(struct elt *e)
{
	// same thing
	return type_vartype_constrain_ari(e);
}

struct stack * type_constrain_cmp(struct elt *e1, struct elt *e2)
{
	if (0 == type_vartype_constrain_cmp(e1)) { return NULL; }
	if (0 == type_vartype_constrain_cmp(e2)) { return NULL; }

	struct stack *types = stack_new();
	stack_push(types, &possible_types[BOO_T]);

	return types;
}

static int type_vartype_constrain_boo(struct elt *e)
{
	int ret = 0;
	struct stack *tmp, *inter;

	// TODO: Init this only once at the begining
	tmp = stack_new();
	stack_push(tmp, &possible_types[FLO_T]);
	stack_push(tmp, &possible_types[INT_T]);
	stack_push(tmp, &possible_types[BOO_T]);

	if (e->elttype == E_REG) {
		inter = type_inter(tmp, e->reg->types);
		ret = stack_size(inter);

		if (ret != 0) {
			// replace old stack with the new one
			reg_settypes(e->reg, inter);
		}

		stack_free(&inter, NULL);
	} 

	if (e->elttype == E_VAR) {
		inter = type_inter(tmp, e->var->t);
		ret = stack_size(inter);

		if (ret != 0) {
			// replace old stack with the new one
			var_put_types(e->var, inter);
		}

		stack_free(&inter, NULL);
	} 
	
	else {
		if (e->cst->type != BOO_T
				&& e->cst->type != INT_T
				&& e->cst->type != FLO_T) {
			ret = 0;
		} else {
			ret = 1;
		}
	}

	stack_free(&tmp, NULL);

	return ret;
}

struct stack * type_constrain_boo(struct elt *e1, struct elt *e2)
{
	if (0 == type_vartype_constrain_boo(e1)) { return NULL; }
	if (0 == type_vartype_constrain_boo(e2)) { return NULL; }

	struct stack *types = stack_new();
	stack_push(types, &possible_types[BOO_T]);

	return types;
}

struct instr * i3addr(char optype, struct elt *e1, struct elt *e2)
{
	struct reg *reg;
	struct instr *i;
	struct stack *types;


	/* DEBUG STUFF
	printf("Opcode: %#x\n", optype);
	elt_dump(e1);
	elt_dump(e2);
	*/

	if (optype & I_ARI) {
		types = type_constrain_ari(e1, e2);
		if (types == NULL) { 
			fprintf(stderr, "Invalid type for arithmetic operation.\n");
			return NULL;
		}
	}
	
	else if (optype & I_CMP) {
		types = type_constrain_cmp(e1, e2);
		if (types == NULL) { 
			fprintf(stderr, "Invalid type for comparison operation.\n");
			return NULL;
		}
	}

	else if (optype & I_BOO) {
		types = type_constrain_boo(e1, e2);
		if (types == NULL) { 
			fprintf(stderr, "Invalid type for boolean operation.\n");
			return NULL;
		}
	}
	
	else {
		fprintf(stderr, "Unrecognized opcode.\n");
		return NULL;
	}

	// TODO: ameliorer le vieux rafistolage
	reg = reg_new(var_new("dummy"));
	reg_settypes(reg, types);
	i = instr_new(optype, NULL, elt_new(E_REG, reg), e1, e2);

	stack_free(&types, NULL);
	return i;
}


struct instr * icast(
	void (*cast_func)(struct elt *, struct elt **),
	struct elt *tocast, type_t t)
{
	struct instr *i = malloc(sizeof *i);

	i->optype = I_CAST;
	i->cast_func = cast_func;
	i->tocast = tocast;
	i->res = elt_new(E_REG, reg_new(NULL));

	stack_push(i->res->reg->types, &possible_types[t]);
	 
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

struct instr * icall(char *fn, struct stack * args)
{
	struct instr *i = malloc(sizeof *i);

	if (i == NULL) {
		perror("malloc");
		return NULL;
	}

	i->optype = I_CAL;
	i->fn = fn;
	i->args = args;
	i->ret = elt_new(E_REG, reg_new(var_new(fn)));

	return i;
}

struct instr * iputs(struct elt *elt)
{
	struct instr *i;
	i = instr_new(I_PUT, NULL, elt, NULL, NULL);

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

	i = instr_new(I_LOA, vr, elt_new(E_REG, reg), NULL, NULL);

	return i;
}

struct instr * istore(struct var *vr, struct elt *elt)
{
	struct instr *i;
	struct stack *typeinter;

	if (elt->elttype == E_REG) {
		typeinter = type_inter(vr->t, elt->reg->types);

		if (stack_size(typeinter) == 0) {
			fprintf(stderr, "Error: Incompatible types in assignment.\n");
			return NULL;
		}

		reg_bind(elt->reg, vr);
		reg_settypes(elt->reg, typeinter);
		stack_free(&typeinter, NULL);

		i = instr_new(I_STO, vr, elt, NULL, NULL);
	}

	else {
		stack_clear(vr->t, NULL);
		stack_push(vr->t, &possible_types[(int)elt->cst->type]);
		i = instr_new(I_STO, vr, elt, NULL, NULL);
	}

	return i;
}

struct instr * idummy(struct var *vr)
{
	struct instr *i;

	i = instr_new(I_DUM, vr, NULL, NULL, NULL);

	return i;
}

struct elt * instr_get_result(const struct instr * i)
{
	struct elt *e;

	switch (i->optype) {
		case I_RAW:
			e = NULL;
			break;
		case I_CAST:
			e = elt_copy(i->res);
			break;
		case I_CAL:
			e = elt_copy(i->ret);
			break;
		case I_DUM:
			e = elt_new(E_VAR, i->vr);
			break;
		default:
			e = elt_copy(i->er);
	}
	
	return e;
}
