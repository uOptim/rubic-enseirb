#include "types.h"
#include "gencode.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_FN_SIZE 250
#define TYPE_NB	5
static char type2mangling[TYPE_NB] = { 'I', 'F', 'B', 'S', 'O' };


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

struct cst * craft_operation(
	const struct cst *,
	const struct cst *,
	const char *,
	const char *);

struct cst * craft_boolean_operation(
	const struct cst *,
	const struct cst *,
	const char *);

int                 craft_store(struct var *, const struct cst *);
static const char * local2llvm_type(char);
static void         gencode_param(void *, void *, void *);
static const char * func_mangling(struct function *);
static void         fn_append(void *, void *, void *);

int gencode_instr(struct instr *i)
{
	printf("Instruction: optype = %#x\n", i->op_type);
	return 0;
}

int gencode_stack(struct stack *s)
{
	struct instr *i;
	struct stack *alloc = stack_new();
	struct stack *other = stack_new();

	/* Note that instruction are poped from the stack in reverse order. We
	 * first sort them in order to separate allocs from other instructions by
	 * pushing them onto two other stacks. As a side effect, this process
	 * reorders instructions in the correct order. */

	while ((i = (struct instr *) stack_pop(s)) != NULL) {
		if (i->op_type == I_ALO) {
			stack_push(alloc, i);
		} else {
			stack_push(other, i);
		}
	}

	// gen allocs first
	while ((i = (struct instr *) stack_pop(alloc)) != NULL) {
		gencode_instr(i);
		stack_push(s, i);
	}

	// then generate other instruction
	while ((i = (struct instr *) stack_pop(other)) != NULL) {
		gencode_instr(i);
		stack_push(s, i);
	}

	stack_free(&other, NULL);
	stack_free(&alloc, NULL);

	return 0;
}

/* Given a function with parameters, local variables and return type 
 * determined, the function code is generated.
 * The code is printed on stdout.
 */
void gencode_func(struct function *f, struct stack *instructions)
{
	int is_first_param = 1;
	assert(params_type_is_known(f));

	printf("define %s @%s(", local2llvm_type(f->ret), func_mangling(f));
	stack_map(f->params, gencode_param, &is_first_param, NULL);
	printf(") {\n");
	gencode_stack(instructions);
	printf("}");
}

void gencode_param(void *param, void *is_first_param, void *dummy) {
	struct var * p = (struct var *)param;
	int * is_fp = (int *)is_first_param;

	if (dummy == NULL) {
		return;
	}

	if (*is_fp) {
		printf("%s %%%s", local2llvm_type(var_gettype(p)), p->vn);
	}
	else {
		printf(", %s %%%s", local2llvm_type(var_gettype(p)), p->vn);
	}
	*is_fp = 0;
}

char convert2bool(struct cst *c)
{
	char v;

	if (c->type == INT_T) {
		if (c->i > 0) v = 1;
		else v = 0;
	} else if (c->type == FLO_T) {
		if (c->f > 0) v = 1;
		else v = 0;
	} else {
		fprintf(stderr, "Incompatible type for boolean conversion\n");
	}

	return v;
}


const char * local2llvm_type(char type)
{
	switch(type) {
		case INT_T:
			return "i32";
			break;
		case FLO_T:
			return "double";
			break;
		case BOO_T:
			return "i1";
			break;
		case STR_T:
			// TODO
			break;
		default:
			break;
	}

	return "UNKNOWN TYPE";
}

int craft_store(struct var *var, const struct cst *c)
{
	if (var_gettype(var) == UND_T) {
		var_pushtype(var, c->type);
	} else {
		var_pushtype(var, compatibility_table
				[(int)var_gettype(var)]
				[(int)c->type]);
	}

	printf("store %s ", local2llvm_type(var_gettype(var)));
	if (c->reg > 0) {
		printf("%%r%d, ", c->reg);
	} else {
		switch (c->type) {
			case INT_T:
				printf("%d, ", c->i);
				break;
			case FLO_T:
				printf("%g, ", c->f);
				break;
			case BOO_T:
				printf("%s, ", (c->c == 0) ? "false" : "true");
				break;
		}
	}
	printf("%s* %%%s\n", local2llvm_type(var_gettype(var)), var->vn);
	return 0;
}

struct cst * craft_operation(
	const struct cst *c1,
	const struct cst *c2,
	const char *op,
	const char *fop)
{
	struct cst *result = NULL;

	if (c1->type == INT_T && c2->type == INT_T) {
		result = cst_new(INT_T, CST_OPRESULT);
		printf("%%r%d = %s i32 ", result->reg, op);
		if (c1->reg > 0) {
			printf("%%r%d", c1->reg);
		} else {
			printf("%d", c1->i);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", c2->reg);
		} else {
			printf("%d", c2->i);
		}
		puts("");
	}

	else if (c1->type == FLO_T && c2->type == FLO_T) {
		result = cst_new(FLO_T, CST_OPRESULT);
		printf("%%r%d = %s double ", result->reg, fop);
		if (c1->reg > 0) {
			printf("%%r%d", c1->reg);
		} else {
			printf("%#g", c1->f);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", c2->reg);
		} else {
			printf("%#g", c2->f);
		}
		puts("");
	}

	else if (c1->type == INT_T && c2->type == FLO_T) {
		unsigned int r;
		result = cst_new(FLO_T, CST_OPRESULT);

		if (c1->reg > 0) {
			// conversion needed
			r = new_reg();
			printf("%%r%d = sitofp i32 %%r%d to double\n", r, c1->reg);
		}

		printf("%%r%d = %s double ", result->reg, fop);

		if (c1->reg > 0) {
			printf("%%r%d", r);
		} else {
			printf("%d.0", c1->i);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", c2->reg);
		} else {
			printf("%#g", c2->f);
		}
		puts("");
	}

	else if (c1->type == FLO_T && c2->type == INT_T) {
		unsigned int r;
		result = cst_new(FLO_T, CST_OPRESULT);

		if (c2->reg > 0) {
			// conversion
			r = new_reg();
			printf("%%r%d = sitofp i32 %%r%d to double\n", r, c2->reg);
		}
		printf("%%r%d = %s double ", result->reg, fop);
		
		if (c1->reg > 0) {
			printf("%%r%d", c1->reg);
		} else {
			printf("%#g", c1->f);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", r);
		} else {
			printf("%d.0", c2->i);
		}
		puts("");
	}

	return result;
}

struct cst * craft_boolean_conversion(const struct cst *c1)
{
	struct cst *c = NULL;

	switch (c1->type) {
		case INT_T:
			c = cst_new(BOO_T, CST_OPRESULT);
			printf("%%r%d = icmp sgt i32 ", c->reg);
			if (c1->reg > 0) {
				printf("%%r%d ", c1->reg);
			} else {
				printf("%d ", c1->i);
			}
			printf(", 0\n");
			break;
		case FLO_T:
			c = cst_new(BOO_T, CST_OPRESULT);
			printf("%%r%d = fcmp sgt double ", c->reg);
			if (c1->reg > 0) {
				printf("%%r%d ", c1->reg);
			} else {
				printf("%#g ", c1->f);
			}
			printf(", 0.0\n");
			break;
		default:
			return NULL;
	}
	
	return c;
}

struct cst * craft_boolean_operation(
	const struct cst *c1,
	const struct cst *c2,
	const char *op)
{
	struct cst *c = cst_new(BOO_T, CST_OPRESULT);

	if (c1->type != BOO_T) {
		c1 = craft_boolean_conversion(c1);
		if (c1 == NULL) return NULL;
	}
	if (c2->type != BOO_T) {
		c2 = craft_boolean_conversion(c2);
		if (c2 == NULL) return NULL;
	}
	
	printf("%%r%d = icmp %s i1 ", c->reg, op);
	if (c1->reg > 0) {
		printf("%%r%d", c1->reg);
	} else {
		printf("%s", (c1->c > 0) ? "true" : "false");
	}
	printf(", ");
	if (c2->reg > 0) {
		printf("%%r%d", c2->reg);
	} else {
		printf("%s", (c2->c > 0) ? "true" : "false");
	}
	puts("");

	return c;
}

// TODO finish him!
void print_instr(struct instr *i)
{
	if (i->op_type & I_ARI) {
		assert(i->s1->type == CST_T && i->s2->type == CST_T);
		craft_operation(
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
	else if (i->op_type == I_ALO) {
		// allo
	}
	else if (i->op_type == I_RET) {
		// ret
	}
	else {
		gencode_instr(i);
	}
}

/* Generates a unique name for a fonction whose parameters type is
 * determined
 */
const char * func_mangling(struct function *f)
{
	char * fn = NULL;
	char buffer[MAX_FN_SIZE] = "";
	int id = 0;

	stack_map(f->params, fn_append, buffer, &id);
	buffer[id] = '\0';

	fn = strdup(buffer);
	if (fn == NULL) {
		perror("strdup");
	}

	return fn;
}

// TODO: use class name instead of obj type
void fn_append(void * variable, void *d, void *i)
{
	struct var *v = (struct var *)variable;
	char * dst = (char *)d;
	int *id = (int *)i;

	assert(*id < MAX_FN_SIZE);

	dst[(*id)++] = type2mangling[var_gettype(v)];
}

