#include "types.h"
#include "gencode.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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

void craft_operation(
	struct elt *,
	const struct elt *,
	const struct elt *,
	const char *,
	const char *);

void craft_boolean_operation(
	const struct elt *,
	const struct elt *,
	const struct elt *,
	const char *);

static const char * local2llvm_type(char);
static void         fn_append(void *, void *, void *);
static void         fnm_append(void *, void *, void *);
static void         print_instr(struct instr *, struct hashmap *);
static int          gencode_stack(struct stack *, struct hashmap *);
static void         print_elt_reg(const struct elt *);
static char *       get_mangle_name(const char *, struct stack *);


int gencode_stack(struct stack *s, struct hashmap *h)
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
		print_instr(i, h);
		stack_push(s, i);
	}

	// then generate other instruction
	while ((i = (struct instr *) stack_pop(other)) != NULL) {
		print_instr(i, h);
		stack_push(s, i);
	}

	stack_free(&other, NULL);
	stack_free(&alloc, NULL);

	return 0;
}

void gencode_main(struct stack *instructions, struct hashmap *h)
{
	int i = 0;
	for (; i < TYPE_NB; i++) {
		printf("declare i32 @puts%c(%s)\n",
				type2mangling[i], 
				local2llvm_type(i));
	}
	puts("");

	printf( "define i32 @main() {\n");

	gencode_stack(instructions, h);
	printf("ret i32 0\n}\n");
}

/* Given a function with parameters, local variables and return type 
 * determined, the function code is generated.
 * The code is printed on stdout.
 */
void gencode_func(
		struct function *f,
		const char * fnm,
		struct stack *instructions,
		struct hashmap * h)
{
	struct var *v;
	struct stack *tmp = stack_new();

	printf("define ");
	if (f->ret == NULL) {
		printf("void");
	} else {
		printf("%s", local2llvm_type(elt_type(f->ret)));
	}

	printf(" @%s(", fnm);

	// TODO: add something here to allocate space for the object passed as
	// implicit argument if needed.

	while ((v = (struct var *) stack_pop(f->params)) != NULL) {
		// DO NOT allocate space for params !
		//stack_push(instructions, ialloca(v));
		stack_push(tmp, v);

		printf("%s %%%s", local2llvm_type(var_gettype(v)), v->vn);
		if (stack_peak(f->params, 0) != NULL) printf(", ");
	}

	stack_move(tmp, f->params);
	stack_free(&tmp, NULL);

	printf(") {\n");
	gencode_stack(instructions, h);
	if (f->ret == NULL) {
		printf("ret void\n");
	}
	printf("}\n\n");
}

void casttobool(struct elt *tocast, struct elt **res)
{
	switch (elt_type(tocast)) {
		case BOO_T:
			printf("%%r%d = and i1 ", (*res)->reg->num);
			print_elt_reg(tocast);
			printf(", true\n");
			break;
		case INT_T:
			printf("%%r%d = icmp ne i32 ", (*res)->reg->num);
			print_elt_reg(tocast);
			printf(", 0\n");
			break;
		case FLO_T:
			printf("%%r%d = fcmp one double ", (*res)->reg->num);
			print_elt_reg(tocast);
			printf(", 0.0\n");
			break;
		default:
			elt_free(*res);
			*res = NULL;
	}
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
			return "i8*";
			break;
		default:
			break;
	}

	return "UNKNOWN TYPE";
}

static void print_args(void *arg, void *arg_pos, void *dummy)
{
	struct elt * a = (struct elt *)arg;

	if (dummy != NULL) {
		return;
	}

	if (*(int *)arg_pos != 0) printf(", ");
	printf("%s ", local2llvm_type(elt_type(a)));
	print_elt_reg(a);
	(*(int *)arg_pos)++;
}

int craft_call(
		const char *fn,
		struct stack * args,
		struct elt * ret,
		struct hashmap * h)
{
	char * fnm = get_mangle_name(fn, args);
	struct function * f = hashmap_get(h, fn);
	struct function * fm = hashmap_get(h, fnm);
	int arg_pos = 0;

	if (f == NULL) {
		fprintf(stderr, "Function %s does not exist\n", fn);
		return 1;
	}

	if (fm == NULL) {
		fprintf(stderr, "Incompatible parameters for function %s, no %s\n", fn, fnm);
		return 1;
	}

	if (fm->ret != NULL) {
		print_elt_reg(ret);
		stack_push(ret->reg->types, &possible_types[elt_type(fm->ret)]);
		printf(" = ");
	}

	printf("call ");
	if (fm->ret == NULL) {
		printf("void");
	} else {
		printf("%s", local2llvm_type(elt_type(fm->ret)));
	}
	printf(" @%s(", fnm);

	stack_map(args, print_args, &arg_pos, NULL);
	printf(")\n");

	free(fnm);

	return 0;
}

int craft_puts(const struct elt *e)
{
	printf("call i32 @puts%c(%s ",
			type2mangling[elt_type(e)], 
			local2llvm_type(elt_type(e)));

	print_elt_reg(e);

	printf(")\n");
	return 0;
}

int craft_ret(const struct elt *e)
{
	printf("ret %s ", local2llvm_type(elt_type(e)));
	print_elt_reg(e);
	printf("\n");

	return 0;
}

int craft_store(struct var *var, const struct elt *e)
{
	printf("store %s ", local2llvm_type(var_gettype(var)));
	print_elt_reg(e);
	printf(", %s* %%%s\n", local2llvm_type(var_gettype(var)), var->vn);
	return 0;
}

int craft_load(struct var *var, const struct elt *e)
{
	printf("%%r%d = load %s* %%%s\n", e->reg->num,
			local2llvm_type(var_gettype(var)),
			var->vn);

	return 0;
}

int craft_alloca(struct var *var)
{
	printf("%%%s = alloca %s\n", var->vn, local2llvm_type(var_gettype(var)));

	return 0;
}

void craft_operation(
	struct elt *result,
	const struct elt *e1,
	const struct elt *e2,
	const char *op,
	const char *fop)
{
	if (elt_type(e1) == INT_T && elt_type(e2) == INT_T) {
		//elt_set_type(result, INT_T);
		printf("%%r%d = %s i32 ", result->reg->num, op);
		print_elt_reg(e1);
		printf(", ");
		print_elt_reg(e2);
		puts("");
	}

	else if (elt_type(e1) == FLO_T && elt_type(e2) == FLO_T) {
		//elt_set_type(result, FLO_T);
		printf("%%r%d = %s double ", result->reg->num, fop);
		print_elt_reg(e1);
		printf(", ");
		print_elt_reg(e2);
		puts("");
	}

	else if (elt_type(e1) == INT_T && elt_type(e2) == FLO_T) {
		struct reg * r;
		if (e1->elttype == E_REG || e1->elttype == E_VAR) {
			// conversion needed
			r = reg_new(NULL);
			printf("%%r%d = sitofp i32 ", r->num);
			print_elt_reg(e1);
			printf(" to double\n");
		}

		//elt_set_type(result, FLO_T);
		printf("%%r%d = %s double ", result->reg->num, fop);

		if (e1->elttype == E_REG || e1->elttype == E_VAR) {
			printf("%%r%d", r->num);
			reg_free(r);
		} else {
			printf("%d.0", e1->cst->i);
		}
		printf(", ");
		print_elt_reg(e2);
		puts("");
	}

	else if (elt_type(e1) == FLO_T && elt_type(e2) == INT_T) {
		struct reg * r;

		if (e2->elttype == E_REG || e2->elttype == E_VAR) {
			// conversion needed
			r = reg_new(NULL);
			printf("%%r%d = sitofp i32 ", r->num);
			print_elt_reg(e2);
			printf(" to double\n");
		}

		//elt_set_type(result, FLO_T);
		printf("%%r%d = %s double ", result->reg->num, fop);
		
		print_elt_reg(e1);
		printf(", ");
		if (e2->elttype == E_REG || e2->elttype == E_VAR) {
			printf("%%r%d", r->num);
			reg_free(r);
		} else {
			printf("%d.0", e2->cst->i);
		}
		puts("");
	}
}

struct elt * craft_boolean_conversion(struct elt *e1)
{
	struct elt *c = NULL;

	switch (elt_type(e1)) {
		case INT_T:
			c = elt_copy(e1);
			stack_push(c->reg->types, &possible_types[BOO_T]);

			printf("%%r%d = icmp ne i32 ", c->reg->num);
			print_elt_reg(e1);
			printf(", 0\n");
			break;
		case FLO_T:
			c = elt_new(E_REG, reg_new(NULL));
			stack_push(c->reg->types, &possible_types[BOO_T]);

			printf("%%r%d = fcmp ne double ", c->reg->num);
			print_elt_reg(e1);
			printf(", 0.0\n");
			break;
		default:
			return NULL;
	}
	
	return c;
}


void craft_boolean_operation(
	const struct elt *er,
	const struct elt *e1,
	const struct elt *e2,
	const char *op)
{
	if (elt_type(e1) != BOO_T) {
		fprintf(stderr, "ERROR: incompatible types for boolean operation\n");
		return;
	}
	if (elt_type(e2) != BOO_T) {
		fprintf(stderr, "ERROR: incompatible types for boolean operation\n");
		return;
	}
	
	printf("%%r%d = %s i1 ", er->reg->num, op);
	print_elt_reg(e1);
	printf(", ");
	print_elt_reg(e2);
	puts("");
}

void print_instr(struct instr *i, struct hashmap *h)
{
	if (i->optype & I_ARI) {
		craft_operation(
				i->er,
				i->e1,
				i->e2,
				local2llvm_instr[i->optype - I_ARI - 1][0],
				local2llvm_instr[i->optype - I_ARI - 1][1]
				);
	}
	else if (i->optype & I_BOO) {
		if (i->optype == I_AND) {
			craft_boolean_operation(i->er, i->e1, i->e2, "and");
		}
		else if (i->optype == I_OR) {
			craft_boolean_operation(i->er, i->e1, i->e2, "or");
		}
	}
	else if (i->optype & I_CMP) {
		craft_operation(
				i->er,
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
	else if (i->optype == I_PUT) {
		craft_puts(i->er);
	}
	else if (i->optype == I_RAW) {
		puts(i->rawllvm);
	}
	else if (i->optype == I_CAST) {
		i->cast_func(i->tocast, &i->res);
	}
	else if (i->optype == I_CAL) {
		craft_call(i->fn, i->args, i->ret, h);
	}
	else if (i->optype == I_DUM) {
		; // do nothing
	}
	else {
		fprintf(stderr, "Error: Instruction not supported\n");
	}
}

void print_elt_reg(const struct elt * e)
{
	if (e->elttype == E_REG) {
		printf("%%r%d", e->reg->num);
	}
	else if (e->elttype == E_VAR) {
		printf("%%%s", e->var->vn);
	} else {
		switch (elt_type(e)) {
			case INT_T:
				printf("%d", e->cst->i);
				break;
			case FLO_T:
				printf("%#g", e->cst->f);
				break;
			case BOO_T:
				printf("%s", (e->cst->c == 0) ? "false" : "true");
				break;
			case STR_T:
				// TODO
				break;
		}
	}
}

/* Generates a unique name for a fonction whose parameters type is
 * determined.
 * Used for function definition.
 */
char * func_mangling(struct function *f)
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

	assert(*id < MAX_FN_SIZE);

	dst[(*id)++] = type2mangling[var_gettype(v)];
}

/* Used for function call.
 */
char * get_mangle_name(const char * fn, struct stack * args)
{
	char * fnm = NULL;
	char buffer[MAX_FN_SIZE] = "";
	int id = 0;

	id += snprintf(buffer, MAX_FN_SIZE, "%s", fn);
	stack_map(args, fnm_append, buffer, &id);
	buffer[id] = '\0';

	fnm = strdup(buffer);
	if (fnm == NULL) {
		perror("strdup");
	}

	return fnm;
}

void fnm_append(void * element, void *d, void *i)
{
	struct elt *e = (struct elt *)element;
	char * dst = (char *)d;
	int *id = (int *)i;

	assert(*id < MAX_FN_SIZE);

	dst[(*id)++] = type2mangling[elt_type(e)];
}
