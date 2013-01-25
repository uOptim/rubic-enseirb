#include "types.h"
#include "gencode.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MAX_FN_SIZE 250
static char type2mangling[TYPE_NB] = { 'I', 'F', 'B', 'S' };


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

struct elt * craft_operation(
	const struct elt *,
	const struct elt *,
	const char *,
	const char *);

struct elt * craft_boolean_operation(
	const struct elt *,
	const struct elt *,
	const char *);

int                 craft_store(struct var *, const struct elt *);
static const char * local2llvm_type(char);
// TODO: remove if unused in the end
//static void         gencode_param(void *, void *, void *);
static const char * func_mangling(struct function *);
static void         fn_append(void *, void *, void *);
static void         print_instr(struct instr *);

static unsigned int new_varid() {
	static unsigned int varid = 0;
	return varid++;
}

// TODO: use print_instr code when finished
int gencode_instr(struct instr *i)
{
	if (i->optype == I_RAW) {
		puts(i->rawllvm);
	} else if (i->optype & I_ARI || i->optype & I_CMP) {
		printf("%%r%d = %#x %s %%r%d, %%r%d\n",
				i->er->reg->num,
				i->optype,
				"i32",
				i->e1->reg->num,
				i->e2->reg->num);
	} else {
		printf("optype = %#x\n", i->optype);
	}
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
		if (i->optype == I_ALO) {
			stack_push(alloc, i);
		} else {
			stack_push(other, i);
		}
	}

	// gen allocs first
	while ((i = (struct instr *) stack_pop(alloc)) != NULL) {
		//gencode_instr(i);
		print_instr(i);
		stack_push(s, i);
	}

	// then generate other instruction
	while ((i = (struct instr *) stack_pop(other)) != NULL) {
		//gencode_instr(i);
		print_instr(i);
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
	struct var *v;
	struct stack *tmp = stack_new();

	//int is_first_param = 1;

	//assert(params_type_is_known(f));

	printf("define");
	if (elt_type(f->ret) == UND_T) {
		printf(" void ");
	} else {
		printf(" %s ", local2llvm_type(elt_type(f->ret)));
	}
	// @david: func mangling will be called before calling this function from
	// the parser. Use f->fn instead.
	printf("@%s(", f->fn);

	// TODO: add something here to allocate space for the object passed as
	// implicit argument if needed.

	while ((v = (struct var *) stack_pop(f->params)) != NULL) {
		// allocate space for params
		stack_push(instructions, ialloca(v));
		stack_push(tmp, v);
	}

	while ((v = (struct var *) stack_pop(tmp)) != NULL) {
		// allocate space for params
		stack_push(f->params, v);

		printf("%s %s", local2llvm_type(var_gettype(v)), v->vn);
		if (stack_peak(tmp, 0) != NULL) printf(", ");
	}
	stack_free(&tmp, NULL);

	// @david: uncomment when stack_map works again
	//stack_map(f->params, gencode_param, &is_first_param, NULL);

	printf(") {\n");
	gencode_stack(instructions);
	printf("ret void\n}");
}

/*
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
*/

char convert2bool(struct elt *c)
{
	char v;

	if (elt_type(c) == INT_T) {
		if (c->cst->i > 0) v = 1;
		else v = 0;
	} else if (elt_type(c) == FLO_T) {
		if (c->cst->f > 0) v = 1;
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

int craft_ret(const struct elt *e)
{
	printf("ret %s %%r%d\n", local2llvm_type(elt_type(e)), e->reg->num);

	return 0;
}

int craft_store(struct var *var, const struct elt *e)
{
	printf("store %s ", local2llvm_type(var_gettype(var)));
	if (e->elttype == E_REG) {
		printf("%%r%d, ", e->reg->num);
	} else {
		switch (elt_type(e)) {
			case INT_T:
				printf("%d, ", e->cst->i);
				break;
			case FLO_T:
				printf("%g, ", e->cst->f);
				break;
			case BOO_T:
				printf("%s, ", (e->cst->c == 0) ? "false" : "true");
				break;
		}
	}
	printf("%s* %%%s\n", local2llvm_type(var_gettype(var)), var->vn);
	return 0;
}

int craft_load(struct var *var, const struct elt *e)
{
	printf("%%r%d = load %s %%%s\n", e->reg->num,
			local2llvm_type(var_gettype(var)),
			var->vn);

	return 0;
}

int craft_alloca(struct var *var)
{
	printf("%%%s = alloca %s\n", var->vn, local2llvm_type(var_gettype(var)));

	return 0;
}

struct elt * craft_operation(
	const struct elt *e1,
	const struct elt *e2,
	const char *op,
	const char *fop)
{
	struct elt *result = NULL;

	if (elt_type(e1) == INT_T && elt_type(e2) == INT_T) {
		result = elt_new(E_REG, reg_new(NULL));
		stack_push(result->reg->types, &possible_types[INT_T]);

		printf("%%r%d = %s i32 ", result->reg->num, op);
		if (e1->elttype == E_REG) {
			printf("%%r%d", e1->reg->num);
		} else {
			printf("%d", e1->cst->i);
		}
		printf(", ");
		if (e2->elttype == E_REG) {
			printf("%%r%d", e2->reg->num);
		} else {
			printf("%d", e2->cst->i);
		}
		puts("");
	}

	else if (elt_type(e1) == FLO_T && elt_type(e2) == FLO_T) {
		result = elt_new(E_REG, reg_new(NULL));
		stack_push(result->reg->types, &possible_types[FLO_T]);

		printf("%%r%d = %s double ", result->reg->num, fop);
		if (e1->elttype == E_REG) {
			printf("%%r%d", e1->reg->num);
		} else {
			printf("%#g", e1->cst->f);
		}
		printf(", ");
		if (e2->elttype == E_REG) {
			printf("%%r%d", e2->reg->num);
		} else {
			printf("%#g", e2->cst->f);
		}
		puts("");
	}

	else if (elt_type(e1) == INT_T && elt_type(e2) == FLO_T) {
		struct reg * r;
		result = elt_new(E_REG, reg_new(NULL));
		stack_push(result->reg->types, &possible_types[FLO_T]);

		if (e1->elttype == E_REG) {
			// conversion needed
			r = reg_new(NULL);
			printf("%%r%d = sitofp i32 %%r%d to double\n", 
					r->num, e1->reg->num);
		}

		printf("%%r%d = %s double ", result->reg->num, fop);

		if (e1->elttype == E_REG) {
			printf("%%r%d", r->num);
			reg_free(r);
		} else {
			printf("%d.0", e1->cst->i);
		}
		printf(", ");
		if (e2->elttype == E_REG) {
			printf("%%r%d", e2->reg->num);
		} else {
			printf("%#g", e2->cst->f);
		}
		puts("");
	}

	else if (elt_type(e1) == FLO_T && elt_type(e2) == INT_T) {
		struct reg * r;
		result = elt_new(E_REG, reg_new(NULL));
		stack_push(result->reg->types, &possible_types[FLO_T]);

		if (e2->elttype == E_REG) {
			// conversion
			r = reg_new(NULL);
			printf("%%r%d = sitofp i32 %%r%d to double\n",
					r->num, e2->reg->num);
		}
		printf("%%r%d = %s double ", result->reg->num, fop);
		
		if (e1->elttype == E_REG) {
			printf("%%r%d", e1->reg->num);
		} else {
			printf("%#g", e1->cst->f);
		}
		printf(", ");
		if (e2->elttype == E_REG) {
			printf("%%r%d", r->num);
			reg_free(r);
		} else {
			printf("%d.0", e2->cst->i);
		}
		puts("");
	}

	return result;
}

struct elt * craft_boolean_conversion(const struct elt *e1)
{
	struct elt *c = NULL;

	switch (elt_type(e1)) {
		case INT_T:
			c = elt_new(E_REG, reg_new(NULL));
			stack_push(c->reg->types, &possible_types[BOO_T]);

			printf("%%r%d = icmp sgt i32 ", c->reg->num);
			if (e1->elttype == E_REG) {
				printf("%%r%d ", e1->reg->num);
			} else {
				printf("%d ", e1->cst->i);
			}
			printf(", 0\n");
			break;
		case FLO_T:
			c = elt_new(E_REG, reg_new(NULL));
			stack_push(c->reg->types, &possible_types[BOO_T]);

			printf("%%r%d = fcmp sgt double ", c->reg->num);
			if (e1->elttype == E_REG) {
				printf("%%r%d ", e1->reg->num);
			} else {
				printf("%#g ", e1->cst->f);
			}
			printf(", 0.0\n");
			break;
		default:
			return NULL;
	}
	
	return c;
}

struct elt * craft_boolean_operation(
	const struct elt *e1,
	const struct elt *e2,
	const char *op)
{
	struct elt *c = elt_new(E_REG, reg_new(NULL));
	stack_push(c->reg->types, &possible_types[BOO_T]);

	if (elt_type(e1) != BOO_T) {
		e1 = craft_boolean_conversion(e1);
		if (e1 == NULL) return NULL;
	}
	if (elt_type(e2) != BOO_T) {
		e2 = craft_boolean_conversion(e2);
		if (e2 == NULL) return NULL;
	}
	
	printf("%%r%d = icmp %s i1 ", c->reg->num, op);
	if (e1->elttype == E_REG) {
		printf("%%r%d", e1->reg->num);
	} else {
		printf("%s", (e1->cst->c > 0) ? "true" : "false");
	}
	printf(", ");
	if (e2->elttype == E_REG) {
		printf("%%r%d", e2->reg->num);
	} else {
		printf("%s", (e2->cst->c > 0) ? "true" : "false");
	}
	puts("");

	return c;
}

// TODO finish him!
void print_instr(struct instr *i)
{
	if (i->optype & I_ARI) {
		craft_operation(
				i->e1,
				i->e2,
				local2llvm_instr[i->optype - I_ARI - 1][0],
				local2llvm_instr[i->optype - I_ARI - 1][1]
				);
	}
	else if (i->optype & I_BOO) {
		if (i->optype == I_AND) {
			craft_boolean_operation(i->e2, i->e2, "and");
		}
		else if (i->optype == I_OR) {
			craft_boolean_operation(i->e2, i->e2, "or");
		}
	}
	else if (i->optype & I_CMP) {
		craft_operation(
				i->e1,
				i->e2,
				local2llvm_instr[i->optype - I_CMP - 1 + CMP_OFF][0],
				local2llvm_instr[i->optype - I_CMP - 1 + CMP_OFF][1]
				);
	}
	else if (i->optype == I_STO) {
		craft_store(i->vr, i->er);
	}
	else if (i->optype == I_LOA) {
		craft_load(i->vr, i->er);
	}
	else if (i->optype == I_ALO) {
		craft_alloca(i->vr);
	}
	else if (i->optype == I_RET) {
		craft_ret(i->er);
	}
	else if (i->optype == I_RAW) {
		puts(i->rawllvm);
	}
	else {
		fprintf(stderr, "Error: Instruction not supported\n");
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

	id += snprintf(buffer, MAX_FN_SIZE, "%s", f->fn);
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
	char c = 'U';

	assert(*id < MAX_FN_SIZE);

	if  (var_gettype(v) != UND_T) {
		c = type2mangling[var_gettype(v)];
	}
	dst[(*id)++] = c;
}

