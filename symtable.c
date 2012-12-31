#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
			printf("object, instance of %s", t->ob.cn);
			break;
		default:
			printf("unknown or invalid type.");
			break;
	}

	puts("");
}

/* name is used when var is OBJ_T, CLA_T or FUN_T.
 * For a OBJ_T, name is the name of the object class.
 * For a CLA_T, name is the name of the class.
 * For a FUN_T, name is the name of the returned object class when the
 * function returns an object.
 */
struct var * var_new(const char *name)
{
	struct var * v = malloc(sizeof *v);

	if (v == NULL) return NULL;

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


void var_set(struct var *v, int type, void *value)
{
	v->tt = type;

	switch (type) {
		case INT_T:
			v->in = *((int *) value);
			break;
		case BOO_T:
			v->bo = *((char *) value);
			break;
		case FLO_T:
			v->fl = *((float *) value);
			break;
		case STR_T:
			v->st = strdup((char *) value);
			break;
		case OBJ_T:
			v->ob.cn = strdup((char *) value);
			break;
		default:
			break;
	}
}

void var_free(void *var)
{
	struct var *v = (struct var *) var;

	if (v->vn != NULL) {
		free(v->vn);
		v->vn = NULL;
	}

	switch (v->tt) {
		case INT_T:
			break;
		case BOO_T:
			break;
		case FLO_T:
			break;
		case STR_T:
			free(v->st);
			break;
		case OBJ_T:
			free(v->ob.cn);
			break;
		default:
			break;
	}

	free(v);
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
	f->params = NULL;

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
