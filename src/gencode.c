#include "types.h"
#include "gencode.h"

#include <stdio.h>

struct cst * craft_operation(
	const struct cst *,
	const struct cst *,
	const char *,
	const char *);

struct cst * craft_boolean_operation(
	const struct cst *,
	const struct cst *,
	const char *);

int craft_store(struct var *, const struct cst *);

int gencode_instr(struct instr *i)
{
	return 0;
}

int gencode_stack(struct stack *s)
{
	struct stack *alloc = stack_new();
	struct stack *other = stack_new();

	struct instr*i;

	while ((i = (struct instr*) stack_pop(s)) != NULL) {
		if (instr_get_optype(i) == I_ALL) {
			stack_push(alloc, i);
		} else {
			stack_push(other, i);
		}
	}

	;

	stack_free(&other, NULL);
	stack_free(&alloc, NULL);

	return 0;
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


static const char * local2llvm_type(char type)
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

