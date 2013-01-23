#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const char compatibility_table[3][3] = 
{ { INT_T, FLO_T, -1 },
  { FLO_T, FLO_T, -1 },
  { -1   , -1   , -1 } };


unsigned int new_reg() {
	// register 0 reserved
	static unsigned int reg = 1;
	return reg++;
}

struct var * var_new(const char *name)
{
	struct var * v = malloc(sizeof *v);

	if (v == NULL) return NULL;

	v->t = stack_new();
	v->vn = strdup(name);
	var_pushtype(v, UND_T);

	return v;
}

void var_free(void *var)
{
	struct var *v = (struct var *) var;

	if (v->vn != NULL) {
		free(v->vn);
		v->vn = NULL;
	}
	if (v->t != NULL) {
		stack_free(&v->t, free);
		v->t = NULL;
	}

	free(v);
}

void var_pushtype(struct var *v, unsigned char t)
{
	unsigned char * vt = malloc(sizeof *vt);
	*vt = t;

	stack_push(v->t, vt);
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



void var_dump(void * var)
{
	struct var * v = (struct var *)var;

	printf("var name: %s\n. Possible types", v->vn);

	type_t * t;
	struct stack *tmp = stack_new();
	while ((t = (type_t *) stack_pop(v->t)) != NULL) {
		printf("%c", *t);
		stack_push(tmp, t);
	}

	stack_move(tmp, v->t);
	stack_free(&tmp, free);
}

struct cst * cst_new(char type, char cst_type)
{
	struct cst *c = malloc(sizeof *c);

	c->type = type;
	c->reg  = (cst_type == CST_PURECST) ? 0 : new_reg();

	return c;
}

struct cst * cst_copy(struct cst *c)
{
	struct cst *copy = malloc(sizeof *copy);

	copy->reg  = c->reg;
	copy->type = c->type;

	switch (c->type) {
		case INT_T: copy->i = c->i; break;
		case FLO_T: copy->f = c->f; break;
		case BOO_T: copy->c = c->c; break;
		case STR_T: copy->s = strdup(c->s); break;
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

void cst_dump(void *cst)
{
	struct cst *c = (struct cst *) cst;

	switch (c->type) {
		case BOO_T:
			printf("BOOL: %s\n", (c->c == 1) ? "true" : "false");
			break;
		case INT_T:
			printf("INT: %d\n", c->i);
			break;
		case FLO_T:
			printf("INT: %g\n", c->f);
			break;
	}
}


struct class * class_new()
{
	struct class *c = malloc(sizeof *c);

	if (c == NULL)
		return NULL;

	c->cn = NULL;
	c->super = NULL;
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

	f->ret = UND_T;
	f->fn = strdup(name);
	f->params = stack_new();

	return f;
}


void function_free(void *function)
{
	struct function *f = (struct function *) function;

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

	free(f);
}


void function_dump(void *function) 
{
	struct var *param;
	struct function *f = (struct function *) function;

	printf("Function: %s\n", f->fn);

	unsigned int i = 0;
	if (f->params != NULL) {
		printf("* Params:\n");
		while (NULL != (param = stack_peak(f->params, i))) {
			var_dump(param);
			i++;
		}
	} else {
		printf("No params");
	}

	printf("* Returns: type %c\n", f->ret);

	puts("");
}
