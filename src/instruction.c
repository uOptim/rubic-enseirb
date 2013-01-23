#include "instruction.h"
#include "types.h"
#include "gencode.h"
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

#define CMP_OFF 4 // first index of comp operations in the following array

static char * local2llvm_instr[10][2] = {
	{"add"     , "fadd"    },
	{"sub"     , "fsub"    },
	{"mul"     , "fmul"    },
	{"sdiv"    , "fdiv"    },
	{"icmp eq" , "fcmp oeq"},
	{"icmp ne" , "fcmp one"},
	{"icmp sle", "fcmp ole"},
	{"icmp sge", "fcmp oge"},
	{"icmp slt", "fcmp olt"},
	{"icmp sgt", "fcmp ogt"}
};

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

struct instr {
	char op_type;

	struct symbol * sr; // returned symbol
	struct symbol * s1;
	struct symbol * s2; // might be unused for some instruction
};


/********************************************************************/
/*                      instroperations                      */
/********************************************************************/

static struct instr* instr_new(
		int op_type,
		struct symbol * sr,
		struct symbol * s1,
		struct symbol * s2)
{
	struct instr*i = malloc(sizeof *i);

	i->sr = sr;
	i->s1 = s1;
	i->s2 = s2;
	i->op_type = op_type;

	return i;
}

void instr_free(struct instr**i)
{
	if ((*i)->sr != NULL) { sym_free(&((*i)->sr)); (*i)->sr = NULL; }
	if ((*i)->s1 != NULL) { sym_free(&((*i)->s1)); (*i)->s1 = NULL; }
	if ((*i)->s2 != NULL) { sym_free(&((*i)->s2)); (*i)->s2 = NULL; }

	*i = NULL;
}

/* Set possible symbol types according to the operation they appear in
*/
void type_constrain(struct instr*i)
{
	if (i->op_type & I_ARI) {
		unsigned char types[2] = {INT_T, FLO_T};

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

void instr_print(struct instr *i)
{
	// TODO print instrcode
	if (i->op_type & I_ARI) {
		assert(i->s1->type == CST_T && i->s2->type == CST_T);
		craft_ari(
				i->s1->cst,
				i->s2->cst,
				local2llvm_instr[i->op_type - I_ARI - 1][0],
				local2llvm_instr[i->op_type - I_ARI - 1][1]
				);
	}
	else if (i->op_type & I_BOO) {
		if (i->op_type == I_AND) {
			craft_boolean_operation(i->s2->cst, i->s2->cst, "and");
		}
		else if (i->op_type == I_OR) {
			craft_boolean_operation(i->s2->cst, i->s2->cst, "or");
		}
		else {
			assert(i->s1->type == CST_T && i->s2->type == CST_T);
			craft_operation(
					i->s1->cst,
					i->s2->cst,
					local2llvm_instr[i->op_type - I_CMP - 1 + CMP_OFF][0],
					local2llvm_instr[i->op_type - I_CMP - 1 + CMP_OFF][1]
					);
		}

	}
	else if (i->op_type == I_STO) {
		assert(i->s1->type == VAR_T && i->sr->type == CST_T);
		craft_store(i->s1->var, i->sr->cst);
	}
}


/**********************************
 * instrcreation functions *
 **********************************/

struct instr* i3addr(char type, struct cst *c1, struct cst *c2)
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

struct instr* iret(struct cst *cr)
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

struct instr* ialloca(struct var *vr)
{
	struct instr*i;

	i = instr_new(
			I_ALL,
			sym_new(VAR_T, vr),
			NULL,
			NULL
			);

	return i;
}

struct instr* iload(struct var *vr)
{
	struct instr*i;

	i = instr_new(
			I_LOA,
			sym_new(CST_T, cst_new(UND_T, CST_OPRESULT)),
			sym_new(VAR_T, vr),
			NULL
			);

	return i;
}

struct instr* istore(struct var *vr, struct cst *c1)
{
	struct instr*i;

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


void instr_dump(const struct instr * i)
{
	if (i->sr == NULL) return;

	char srtype = i->sr->type;

	switch (srtype) {
		case CST_T: printf("reg%d", i->sr->cst->reg); break;
		case VAR_T: printf("%s", i->sr->var->vn); break;
	}

	if (i->s1 == NULL) return;
	printf(" = ");

	if (i->s1->cst->reg > 0) {
		printf("reg%d", i->s1->cst->reg);
	} else {
		switch (i->s1->cst->type) {
			case INT_T: printf("%d", i->s1->cst->i); break;
			case FLO_T: printf("%g", i->s1->cst->f); break;
			case UND_T: printf("UND_T"); break;
			default: printf("Gné??");
		}
	}

	printf(" ");

	switch (i->op_type) {
		case I_ADD: printf("+");  break;
		case I_SUB: printf("-");  break;
		case I_MUL: printf("*");  break;
		case I_DIV: printf("/");  break;
		case I_OR:  printf("||"); break;
		case I_AND: printf("&&"); break;
		case I_GT:  printf(">");  break;
		case I_LT:  printf("<");  break;
		case I_GEQ: printf(">="); break;
		case I_LEQ: printf("<="); break;
		case I_EQ:  printf("=="); break;
		case I_NEQ: printf("!="); break;
	}

	if (i->s2 == NULL) return;
	printf(" ");

	if (i->s2->cst->reg > 0) {
		printf("reg%d", i->s2->cst->reg);
	} else {
		switch (i->s2->cst->type) {
			case INT_T: printf("%d", i->s2->cst->i); break;
			case FLO_T: printf("%g", i->s2->cst->f); break;
			case UND_T: printf("UND_T"); break;
			default: printf("Gné??");
		}
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
