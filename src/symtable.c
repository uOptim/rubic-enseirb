#include "symtable.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const char compatibility_table[3][3] = 
{ { INT_T, FLO_T, -1 },
  { FLO_T, FLO_T, -1 },
  { -1   , -1   , -1 } };


struct var * var_new(const char *name)
{
	struct var * v = malloc(sizeof *v);

	if (v == NULL) return NULL;

	v->vn = strdup(name);
	v->t = stack_new();
	type_init(v->t);

	return v;
}

struct var * var_copy(struct var *src)
{
	struct var *dst;
	
	if (src == NULL) {
		dst = NULL;
	} else {
		dst = malloc(sizeof *dst);
		dst->vn = strdup(src->vn);
		dst->t = stack_copy(src->t, type_copy);
	}

	return dst;
}


void var_free(void *var)
{
	struct var *v = (struct var *) var;

	if (v->vn != NULL) {
		free(v->vn);
		v->vn = NULL;
	}
	if (v->t != NULL) {
		stack_free(&v->t, NULL);
		v->t = NULL;
	}

	free(v);
}


/* Returns the first possible variable type
*/
type_t var_gettype(struct var *v)
{
	return *((type_t *)stack_peak(v->t, 0));
}

/* Returns the number of types possible for a variable
*/
int var_type_card(struct var *v)
{
	return stack_size(v->t);
}

int var_isconst(const struct var *v)
{
	if (v->vn != NULL && v->vn[0] >= 'A' && v->vn[0] <= 'Z')
		return 1;

	return 0;
}

int var_isglobal(const struct var *v)
{
	if (v->vn != NULL && v->vn[0] == '$')
		return 1;

	return 0;
}


struct elt * elt_new(char elttype, void *eltptr)
{
	struct elt *elt = malloc(sizeof *elt);

	elt->elttype = elttype;

	switch (elttype) {
		case E_CST:
			elt->cst = eltptr;
			break;
		case E_REG:
			elt->reg = eltptr;
			break;
		default:
			free(elt);
			elt = NULL;
	};

	return elt;
}


void elt_free(void *e)
{
	struct elt *elt = (struct elt *) e;

	switch (elt->elttype) {
		case E_CST:
			cst_free(elt->cst);
			break;
		case E_REG:
			reg_free(elt->reg);
			break;
		default:
			break;
	}

	free(elt);
}

void * elt_copy(void *element)
{
	struct elt *copy;
	struct elt *elt = (struct elt *)element;

	if (elt == NULL) {
		copy = NULL;
	}

	else {
		switch (elt->elttype) {
			case E_CST:
				copy = elt_new(E_CST, cst_copy(elt->cst));
				break;
			case E_REG:
				copy = elt_new(E_REG, reg_copy(elt->reg));
				break;
			default:
				copy = NULL;
		}
	}

	return (void *)copy;
}

void elt_dump(const struct elt *e)
{
	if (e == NULL) {
		puts("Element is NULL!");
	} else {
		switch (e->elttype) {
			case E_CST:
				cst_dump(e->cst);
				break;
			case E_REG:
				reg_dump(e->reg);
				break;
			default:
				puts("Element type unknown.");
		}
	}
}


type_t elt_type(const struct elt * e) {
	if (e->elttype == E_CST) {
		return e->cst->type;
	}
	else {
		if (stack_size(e->reg->types) > 1) {
			// @Benoît: lors de la génération de fonctions, c'est
			// volontairement que les paramètres ont plusieurs types et que le
			// premier et pris comme type par défaut
			//fprintf(stderr, "Warning, multiple types found! Using the first one by default.\n");
		}
		return *(type_t *)stack_peak(e->reg->types, 0);
	}
}

struct reg * reg_new(struct var *v)
{
	static unsigned int reg = 1;

	struct reg *r = malloc(sizeof *r);

	r->num = reg++;
	r->bound = 0;
	r->types = NULL;
	reg_bind(r, v);

	return r;
}

void reg_bind(struct reg *r, struct var *v)
{
	if (r->bound) {
		fprintf(stderr, "Warning: attempting to reuse a register"
		                "already bound to a variable");
		return;
	}

	else if (r->types != NULL) {
		stack_free(&r->types, NULL);
	}

	if (v != NULL) {
		r->bound = 1;
		r->types = v->t;
	} else {
		r->bound = 0;
		r->types = stack_new();
	}
}

void reg_settypes(struct reg *r, struct stack *types)
{
	type_t *t;

	stack_clear(r->types, NULL);

	stack_rewind(types);
	while ((t = stack_next(types)) != NULL) {
		stack_push(r->types, t);
	}
}

void reg_free(struct reg *r)
{
	if (!r->bound) {
		stack_free(&r->types, NULL);
	}

	free(r);
}

struct reg * reg_copy(struct reg *r)
{
	struct reg *copy;
	
	if (r == NULL) {
		copy = NULL;
	} else {
		copy = malloc(sizeof *copy);

		copy->num = r->num;
		copy->bound = r->bound;

		if (r->bound) {
			copy->types = r->types;
		} else {
			copy->types = stack_copy(r->types, type_copy);
		}
	}

	return copy;
}
	
void reg_dump(const struct reg *r)
{
	type_t *t;

	puts("Register:");
	printf("* regnum: %d\n", r->num);
	printf("* types: ");

	stack_rewind(r->types);
	while ((t = stack_next(r->types)) != NULL) {
		printf("%d ", *t);
	}
	puts("");
}

struct cst * cst_new(type_t type)
{
	struct cst *c = malloc(sizeof *c);
	c->type = type;
	return c;
}


struct cst * cst_copy(struct cst *c)
{
	struct cst *copy = malloc(sizeof *copy);

	copy->type = c->type;

	switch (c->type) {
		case INT_T: copy->i = c->i; break;
		case FLO_T: copy->f = c->f; break;
		case BOO_T: copy->c = c->c; break;
		case STR_T: copy->s = strdup(c->s); break;
		default: copy->o = c->o; break;
	}

	return copy;
}

void cst_free(void *cst)
{
	struct cst *c = (struct cst *) cst;

	if (c->type == STR_T) {
		free(c->s);
		c->s = NULL;
	}

	free(cst);
}

void cst_dump(const struct cst *c)
{
	puts("Constant:");

	switch (c->type) {
		case INT_T:
			printf("* type: int, value: %d\n", c->i);
			break;
		case FLO_T:
			printf("* type: float, value: %g\n", c->f);
			break;
		case BOO_T:
			printf("* type: bool, value: %i\n", c->c);
			break;
		default:
			printf("Type Unknown\n");
	}
}

struct class * class_new(const char *name)
{
	struct class *c = malloc(sizeof *c);

	if (c == NULL)
		return NULL;

	c->cn = strdup(name);
	c->super = NULL;
	c->typenum = type_new();
	c->attrs = hashmap_new();
	c->methods = hashmap_new();

	return c;
}


void class_free(void *class)
{
	struct class *c = (struct class *) class;

	if (c->cn != NULL) {
		free(c->cn);
		c->cn = NULL;
	}

	if (c->methods != NULL) {
		hashmap_free(&c->methods, function_free);
		c->methods = NULL;
	}

	if (c->attrs != NULL) {
		hashmap_free(&c->attrs, var_free);
		c->attrs = NULL;
	}

	c->super = NULL;

	free(class);
}


void class_dump(void *class)
{
	struct class *c = (struct class *) class;

	printf("Class: %s\n", c->cn);

	if (c->super != NULL) {
		printf("Super class: %s\n", c->super->cn);
	}
}


struct function * function_new(const char *name)
{
	struct function *f = malloc(sizeof *f);

	if (f == NULL)
		return NULL;

	f->ret = NULL;
	f->fn = strdup(name);
	f->params = stack_new();

	return f;
}

void function_free(void *function)
{
	struct function *f = (struct function *) function;

	if (f == DUMMY_FUNC) {
		return;
	}

	if (f->fn != NULL) {
		free(f->fn);
		f->fn = NULL;
	}

	if (f->params != NULL) {
		// give NULL otherwise we get double frees (since params are put in
		// the scope block too
		stack_free(&f->params, NULL);
		f->params = NULL;
	}

	if (f->ret != NULL) {
		elt_free(f->ret);
	}

	free(f);
}

void function_dump(void *function)
{
	struct function *f = (struct function *) function;

	printf("Function: %s\n", f->fn);

	printf("Ret:\n");
	elt_dump(f->ret);

	printf("Params:\n");
}
