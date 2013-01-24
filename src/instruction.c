#include "instruction.h"
#include "types.h"
#include "gencode.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static struct symbol * sym_new(char, void *);
static void            sym_free(struct symbol **);



static struct instr* instr_new(
		int op_type,
		struct var * vr,
		struct cst * cr,
		struct cst * c1,
		struct cst * c2)
{
	if (op_type == I_RAW) {
		return NULL;
	}

	struct instr *i = malloc(sizeof *i);

	i->vr = vr;
	i->cr = cr;
	i->c1 = c1;
	i->c2 = c2;
	i->op_type = op_type;

	return i;
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
		if (i->cr != NULL) {
			cst_free(i->cr);
			i->cr = NULL;
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
void type_constrain(struct instr *i)
{
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
	struct cst *cr;
	struct instr *i;

	cr = cst_new(UND_T, CST_OPRESULT);

	if (optype & I_ARI || optype & I_CMP) {
		if (c1->type > FLO_T || c2->type > FLO_T) {
			fprintf(stderr, "Incomatible types for operation.\n");
			return NULL;
		}

		cr->type = compatibility_table[(int)c1->type][(int)c2->type];
	} 
	
	else if (optype & I_BOO) {
		if (c1->type != BOO_T || c2->type != BOO_T) {
			fprintf(stderr, "Incompatible types for boolean operation.\n");
			return NULL;
		}

		cr->type = BOO_T;
	}
	
	else {
		fprintf(stderr, "Unrecognized types.\n");
		return NULL;
	}

	i = instr_new(optype, NULL, cr, c1, c2);

	return i;
}

struct instr * iret(struct cst *cr)
{
	struct instr *i;
	i = instr_new(I_RET, NULL, cr, NULL, NULL);

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

	i = instr_new(
			I_LOA, 
			vr,
			cst_new(var_gettype(vr), CST_OPRESULT),
			NULL,
			NULL
		);

	return i;
}

struct instr * istore(struct var *vr, struct cst *c1)
{
	type_t *t;
	struct instr *i;
	struct stack *tmp;

	tmp = stack_new();
	i = instr_new(I_STO, vr, c1, NULL, NULL);

	// if variable not yet defined
	if (var_gettype(vr) == UND_T) {
		stack_clear(vr->t, free);
		var_pushtype(vr, c1->type);
	}

	else {
		while ((t = stack_pop(vr->t)) != NULL) {
			stack_push(tmp, t);
			if (*t == c1->type) break;
		}

		if (t == NULL) {
			instr_free(i);
			i = NULL;
			// restore type stack
			stack_move(tmp, vr->t);
			fprintf(stderr, "Error: Incompatible types in assignment\n");
		} else {
			stack_clear(vr->t, free);
			var_pushtype(vr, c1->type);
		}
	}

	stack_free(&tmp, free);

	return i;
}


struct cst * instr_get_result(const struct instr * i)
{
	return cst_copy(i->cr);
}


struct symbol * sym_new(char t, void *d)
{
	struct symbol *s = malloc(sizeof *s);

	s->type = t;

	switch (t) {
		case VAR_T:
			s->var = (struct var *) d;
			break;
		case CST_T:
			s->cst = (struct cst *) d;
			break;
		default:
			return NULL;
	}

	return s;
}

void sym_free(struct symbol **s)
{
	switch ((*s)->type) {
		case VAR_T:
			var_free((*s)->var);
			break;
		case CST_T:
			cst_free((*s)->cst);
			break;
	}

	free(*s);
	*s = NULL;
}
