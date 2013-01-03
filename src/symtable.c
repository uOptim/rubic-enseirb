#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


struct symbol * sym_new(const char *name, char t, void *d)
{
	struct symbol *s = malloc(sizeof *s);

	s->type = t;
	s->name = strdup(name);
	s->ptr = d;

	return s;
}

void sym_free(void *sym)
{
	struct symbol *s = (struct symbol *) sym;
	free(s->name);

	switch (s->type) {
		case VAR_T:
			var_free(s->ptr);
			break;
		case FUN_T:
			function_free(s->ptr);
			break;
		case CLA_T:
			class_free(s->ptr);
			break;
		default:
			break;
	}

	free(s);
}


void sym_dump(void *sym)
{
	struct symbol *s = (struct symbol *) sym;
	printf("Symbol name: %s\n", s->name);

	switch (s->type) {
		case VAR_T:
			var_dump((struct var *) s->ptr);
			break;
		case FUN_T:
			function_dump((struct function *) s->ptr);
			break;
		case CLA_T:
			class_dump((struct class *) s->ptr);
			break;
		default:
			break;
	}
}


struct var * var_new(const char *name, unsigned int reg)
{
	struct var * v = malloc(sizeof *v);

	if (v == NULL) return NULL;

	v->reg = reg;
	v->tt = UND_T;
	v->vn = strdup(name);
	/* Constants start with a capital letter */
	if (name[0] > 'A' && name[0] < 'Z') {
		v->tc = 1;
	}
	else {
		v->tc = 0;
	}

	return v;
}


void var_free(void *var)
{
	struct var *v = (struct var *) var;

	if (v->vn != NULL) {
		free(v->vn);
		v->vn = NULL;
	}

	free(v);
}


void var_dump(void * var)
{
	struct var * t = (struct var *)var;

	printf("var name: %s", t->vn);

	printf(" - ");

	switch (t->tt) {
		case INT_T:
			printf("integer");
			break;
		case BOO_T:
			printf("boolean");
			break;
		case FLO_T:
			printf("floating point number");
			break;
		case STR_T:
			printf("string");
			break;
		case OBJ_T:
			printf("object");
			break;
		default:
			printf("unknown type.");
			break;
	}

	puts("");
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


struct function * function_new()
{
	struct function *f = malloc(sizeof *f);

	if (f == NULL)
		return NULL;

	f->fn = NULL;
	f->ret = NULL;
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

	if (f->ret != NULL) {
		var_free(f->ret);
		f->ret = NULL;
	}

	if (f->params != NULL) {
		stack_free(&f->params, var_free);
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

	if (f->ret != NULL) {
		printf("* Returns:\n");
		var_dump(f->ret);
	} else {
		printf("No return");
	}

	puts("");
}
