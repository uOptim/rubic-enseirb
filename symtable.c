#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NB_T 6

static const char tt[NB_T][4] = {"FUN", "CLA", "OBJ", "INT", "FLO", "STR"};

void type_dump(void * type) {
	struct type * t = (struct type *)type;

	switch (t->tt) {
		case INT_T:
			puts("intger");
			break;
		case FLO_T:
			puts("floating point number");
			break;
		case STR_T:
			puts("String");
			break;
		case OBJ_T:
			printf("object, instance of %s\n", t->ob.cn);
			break;
		default:
			fprintf(stderr, "Invalid type.\n");
			break;
	}
}

/* name is used when type is OBJ_T, CLA_T or FUN_T.
 * For a OBJ_T, name is the name of the object class.
 * For a CLA_T, name is the name of the class.
 * For a FUN_T, name is the name of the returned object class when the
 * function returns an object.
 */
struct type * type_new(int type, void *data) {
	struct type * t = malloc(sizeof *t);

	if (t == NULL)
		return NULL;

	t->t = type;

	switch (type) {
		case INT_T:
			t->in = *((int *) data);
			break;
		case FLO_T:
			t->fl = *((float *) data);
			break;
		case STR_T:
			t->st = strdup((char *) data);
			break;
		case OBJ_T:
			t->ob.cn = strdup((char *) data);
			break;
		default:
			break;
	}

	return t;
}

void type_free(void * type) {
	struct type **t = (struct type **) type;

	switch ((*t)->tt) {
		case INT_T:
			break;
		case FLO_T:
			break;
		case STR_T:
			free((*t)->st);
			break;
		case OBJ_T:
			free((*t)->ob.cn);
			break;
		default:
			break;
	}

	free(*t);
	*t = NULL;
}


struct class * class_new(const char *name, struct class *super)
{
	struct class *c = malloc(sizeof *c);

	if (c == NULL)
		return NULL;

	c->cn = strdup(name);
	c->super = super;
	c->methods = stack_new();

	return c;
}


void class_free(void *class)
{
	struct class *c = (struct class *) class;

	free(c->cn);
	assert(c->methods != NULL);
	stack_free(&c->methods, NULL);

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


struct function * function_new(const char *name, struct stack *params)
{
	struct function *f = malloc(sizeof *f);

	if (f == NULL)
		return NULL;

	f->ret = malloc(sizeof *(f->ret));

	if (f->ret == NULL) {
		free(f);
		return NULL;
	}

	f->fn = strdup(name);
	f->params = params;

	assert(f->fn != NULL);
	assert(f->params != NULL);

	return f;
}


void function_free(void *function)
{
	struct function *f = (struct function *) function;

	free(f->fn);
	free(f->ret);
	stack_free(&f->params, free); //TODO "free" must be replaced

	free(f);
}


void function_dump(void *function) 
{
	struct function *f = (struct function *) function;

	printf("Function: %s\n", f->fn);
	// params?
}