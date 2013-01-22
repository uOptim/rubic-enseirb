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

static void instruction_free(struct instruction **i)
{
	sym_free(&((*i)->sr));
	sym_free(&((*i)->s1));
	sym_free(&((*i)->s2));

	*i = NULL;
}


/**********************************
 * instruction creation functions *
 **********************************/

struct instruction * i_add(struct cst *cr, struct cst *c1, struct cst *c2)
{
	struct instruction *i;
	i = instr_new(I_ADD, sym_new(CST_T, cr), sym_new(CST_T, c1), sym_new(CST_T, c2));
	return i;
}

struct instruction * i_sub(struct cst *cr, struct cst *c1, struct cst *c2)
{
	struct instruction *i;
	i = instr_new(I_SUB, sym_new(CST_T, cr), sym_new(CST_T, c1), sym_new(CST_T, c2));
	return i;
}

struct instruction * i_mul(struct cst *cr, struct cst *c1, struct cst *c2)
{
	struct instruction *i;
	i = instr_new(I_MUL, sym_new(CST_T, cr), sym_new(CST_T, c1), sym_new(CST_T, c2));
	return i;
}

struct instruction * i_div(struct cst *cr, struct cst *c1, struct cst *c2)
{
	struct instruction *i;
	i = instr_new(I_DIV, sym_new(CST_T, cr), sym_new(CST_T, c1), sym_new(CST_T, c2));
	return i;
}

struct instruction * i_store(struct var *vr, struct cst *c1)
{
	struct instruction *i;
	i = instr_new(I_STO, sym_new(VAR_T, vr), sym_new(CST_T, c1), NULL);
	return i;
}

struct instruction * i_load(struct var *vr, struct cst *c1)
{
	struct instruction *i;
	i = instr_new(I_LOA, sym_new(VAR_T, vr), sym_new(CST_T, c1), NULL);
	return i;
}

struct instruction * i_assign(struct var *vr, struct cst *c1)
{
	return NULL;
}


void instruction_dump(const struct instruction* i)
{
	switch(i->sr->type) {
		case CST_T:
			printf("reg");
			break;
		case VAR_T:
			printf("%s", i->sr->var->vn);
			break;
	}

	printf(" = constant ");
		
	switch(i->op_type) {
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
	}

	printf("constant\n");
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
