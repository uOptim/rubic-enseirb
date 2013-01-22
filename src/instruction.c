#include "instruction.h"

#include <stdio.h>
#include <stdlib.h>

// symbols
struct symbol {
	char type;
	union {
		struct cst *cst;
		struct var *var;
	};
};

static struct symbol * sym_new(char, void *);
static void            sym_free(struct symbol **);

struct instruction {
	char op_type;

	struct symbol * sr; // returned symbol
	struct symbol * s1;
	struct symbol * s2; // might be unused for some instruction
};


/********************************************************************/
/*                      Instruction operations                      */
/********************************************************************/

static struct instruction * instr_new(
	int op_type,
	struct symbol * sr,
	struct symbol * s1,
	struct symbol * s2)
{
	struct instruction *i = malloc(sizeof *i);

	i->sr = sr;
	i->s1 = s1;
	i->s2 = s2;
	i->op_type = op_type;

	return i;
}

void instruction_free(struct instruction **i)
{
	if ((*i)->sr != NULL) sym_free(&((*i)->sr));
	if ((*i)->s1 != NULL) sym_free(&((*i)->s1));
	if ((*i)->s2 != NULL) sym_free(&((*i)->s2));

	*i = NULL;
}


/**********************************
 * instruction creation functions *
 **********************************/

struct instruction * i3addr(char type, struct cst *c1, struct cst *c2)
{
	struct instruction *i;

	i = instr_new(
			type,
			sym_new(CST_T, cst_new(UND_T, CST_OPRESULT)),
			sym_new(CST_T, c1),
			sym_new(CST_T, c2)
	);

	return i;
}

struct instruction * i_load(struct var *vr)
{
	struct instruction *i;

	i = instr_new(
			I_LOA,
			sym_new(CST_T, cst_new(UND_T, CST_OPRESULT)),
			sym_new(VAR_T, vr),
			NULL
	);
	return i;
}

struct instruction * i_store(struct var *vr, struct cst *c1)
{
	return NULL;
}


struct cst * instruction_get_result(const struct instruction *i)
{
	if (i->sr->type == CST_T) {
		return i->sr->cst;
	}

	return NULL;
}


void instruction_dump(const struct instruction* i)
{
	switch (i->sr->type) {
		case CST_T:
			printf("reg");
			break;
		case VAR_T:
			printf("%s", i->sr->var->vn);
			break;
	}

	printf(" = ");

	switch (i->s1->cst->type) {
		case INT_T:
			printf("%d", i->s1->cst->i);
			break;
		case FLO_T:
			printf("%g", i->s1->cst->f);
			break;
	}

	printf(" ");

	switch (i->op_type) {
		case I_ADD:
			printf("+");
			break;
		case I_SUB:
			printf("-");
			break;
		case I_MUL:
			printf("*");
			break;
		case I_DIV:
			printf("*");
			break;
		case I_OR:
			printf("||");
			break;
	}

	printf(" ");

	switch (i->s2->cst->type) {
		case INT_T:
			printf("%d", i->s2->cst->i);
			break;
		case FLO_T:
			printf("%g", i->s2->cst->f);
			break;
	}

	printf("\n");
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
