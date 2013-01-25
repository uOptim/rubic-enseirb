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
			i->vr,
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

/* Set possible symbol types according to the operation they appear in
*/
// TODO
void instr_constrain(void *instruction, void *dummy1, void *dummy2)
{
	/*
	struct instr *i = (struct instr *)instruction;
	if (dummy1 != NULL || dummy2 != NULL) {
		return;
	}
	if (i->optype & I_ARI) {
		type_t types[2] = {INT_T, FLO_T};

		type_inter(i->e1->var, types, 2);
		type_inter(i->e2->var, types, 2);
		if (var_type_card((struct var*)i->e1->var) == 1
				&& var_type_card((struct var*)i->e2->var) == 1) {
			// Common type for variables stored in e1 and e2
			types[0] = compatibility_table
				[var_gettype((struct var *)i->e1->var)]
				[var_gettype((struct var *)i->e2->var)];
			type_inter(i->er->var, types, 1);
		}
		else {
			type_inter(i->er->var, types, 2);
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

	i->optype = I_RAW;
	i->rawllvm = strdup(s);

	if (i->rawllvm == NULL) {
		perror("strdup");
		free(i);
		i = NULL;
	}

	return i;
}


static int verif_type(struct elt *e, type_t type)
{
	if (e->elttype == E_CST && e->cst->type == type) {
		return E_CST;
	} else if (type_ispresent(e->reg->types, FLO_T)) {
		return E_REG;
	}

	return -1;
}

struct instr * i3addr(char optype, struct elt *e1, struct elt *e2)
{
	struct instr *i;
	struct elt *elt;
	struct reg *reg;

	if (optype & I_ARI) {
		char nbfloat = 0;

		if (verif_type(e1, FLO_T) != -1) {
			nbfloat++;
		} else if (verif_type(e1, INT_T) != -1) {
			;
		} else {
			fprintf(stderr, "Invalid type for arithmetic operation\n");
			return NULL;
		}

		if (verif_type(e2, FLO_T) != -1) {
			nbfloat++;
		} else if (verif_type(e2, INT_T) != -1) {
			;
		} else {
			fprintf(stderr, "Invalid type for arithmetic operation\n");
			return NULL;
		}

		reg = reg_new(NULL);
		if (nbfloat > 0) {
			stack_push(reg->types, &possible_types[FLO_T]);

			if (e1->elttype == E_REG) {
				stack_clear(e1->reg->types, NULL);
				stack_push(e1->reg->types, &possible_types[FLO_T]);
			}

			if (e2->elttype == E_REG) {
				stack_clear(e2->reg->types, NULL);
				stack_push(e2->reg->types, &possible_types[FLO_T]);
			}
		} else {
			stack_push(reg->types, &possible_types[FLO_T]);
			stack_push(reg->types, &possible_types[INT_T]);

			if (e1->elttype == E_REG) {
				stack_clear(e1->reg->types, NULL);
				stack_push(e1->reg->types, &possible_types[FLO_T]);
				stack_push(e1->reg->types, &possible_types[INT_T]);
			}

			if (e2->elttype == E_REG) {
				stack_clear(e2->reg->types, NULL);
				stack_push(e2->reg->types, &possible_types[FLO_T]);
				stack_push(e2->reg->types, &possible_types[INT_T]);
			}
		}

		elt = elt_new(E_REG, reg);
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

	i = instr_new(optype, NULL, elt, e1, e2);

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
	i = instr_new(I_ALO, vr, elt_new(E_REG, vr), NULL, NULL);

	return i;
}

struct instr * iload(struct var *vr)
{
	struct instr *i;

	i = instr_new(
			I_LOA, 
			vr,
			elt_new(E_REG, vr),
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
