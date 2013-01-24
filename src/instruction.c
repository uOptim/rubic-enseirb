#include "instruction.h"
#include "types.h"
#include "gencode.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static struct symbol * sym_new(char, void *);
static void            sym_free(struct symbol **);



/********************************************************************/
/*                      instroperations                      */
/********************************************************************/

static struct instr* instr_new(
		int op_type,
		struct symbol * sr,
		struct symbol * s1,
		struct symbol * s2)
{
	if (op_type == I_RAW) {
		return NULL;
	}

	struct instr *i = malloc(sizeof *i);

	i->sr = sr;
	i->s1 = s1;
	i->s2 = s2;
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
		if (i->sr != NULL) {
			if (i->sr->type != VAR_T) { 
				sym_free(&i->sr);
			} else {
				free(i->sr);
			}
		}
		if (i->s1 != NULL) {
			if (i->s1->type != VAR_T) {
				sym_free(&i->s1);
			} else {
				free(i->s1);
			}
		}
		if (i->s2 != NULL) {
			if (i->s2->type != VAR_T) {
				sym_free(&i->s2);
			} else {
				free(i->s2);
			}
		}

		i->sr = NULL;
		i->s1 = NULL;
		i->s2 = NULL;
	}

	free(i);
}

/* Set possible symbol types according to the operation they appear in
*/
void type_constrain(struct instr*i)
{
	if (i->op_type & I_ARI) {
		type_t types[2] = {INT_T, FLO_T};

		type_inter(i->s1->var, types, 2);
		type_inter(i->s2->var, types, 2);
		if (var_type_card((struct var*)i->s1->var) == 1
				&& var_type_card((struct var*)i->s2->var) == 1) {
			// Common type for variables stored in s1 and s2
			types[0] = compatibility_table
				[var_gettype((struct var *)i->s1->var)]
				[var_gettype((struct var *)i->s2->var)];
			type_inter(i->sr->var, types, 1);
		}
		else {
			type_inter(i->sr->var, types, 2);
		}
	}
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


struct instr * i3addr(char type, struct cst *c1, struct cst *c2)
{
	struct cst *cr;
	struct instr*i;

	cr = cst_new(UND_T, CST_OPRESULT);

	if (type & I_ARI) {
		cr->type = compatibility_table[(int)c1->type][(int)c2->type];
	} else if (type & I_BOO) {
		cr->type = BOO_T;
	} else {
		fprintf(stderr, "Unrecognized types\n");
		return NULL;
	}

	i = instr_new(
			type,
			sym_new(CST_T, cr),
			sym_new(CST_T, c1),
			sym_new(CST_T, c2)
		);

	return i;
}

struct instr * iret(struct cst *cr)
{
	struct instr*i;

	i = instr_new(
			I_RET,
			sym_new(CST_T, cr),
			NULL,
			NULL
		);

	return i;
}

struct instr * ialloca(struct var *vr)
{
	struct instr *i;

	i = instr_new(
			I_ALO,
			sym_new(VAR_T, vr),
			NULL,
			NULL
		);

	return i;
}

struct instr * iload(struct var *vr)
{
	struct instr *i;

	i = instr_new(
			I_LOA,
			sym_new(CST_T, cst_new(UND_T, CST_OPRESULT)),
			sym_new(VAR_T, vr),
			NULL
		);

	return i;
}

struct instr * istore(struct var *vr, struct cst *c1)
{
	struct instr *i;

	i = instr_new(
			I_STO,
			sym_new(CST_T, c1),
			sym_new(VAR_T, vr),
			NULL
		);

	return i;
}


struct cst * instr_get_result(const struct instr * i)
{
	if (i->sr->type == CST_T) {
		return cst_copy(i->sr->cst);
	}

	return NULL;
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
