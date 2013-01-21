#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

const char compatibility_table[3][3] = 
{ { INT_T, FLO_T, BOO_T },
  { FLO_T, FLO_T, BOO_T },
  { BOO_T, BOO_T, BOO_T } };


unsigned int new_reg() {
	// register 0 reserved
	static unsigned int reg = 1;
	return reg++;
}


struct symbol * sym_new(const char *name, char t, void *d)
{
	struct symbol *s = malloc(sizeof *s);

	s->ptr = d;
	s->type = t;
	s->name = strdup(name);

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

struct type * type_new(unsigned char t) {
	struct type * ty = malloc(sizeof *t);

	if (ty == NULL) return NULL;

	ty->t = t;
	return t;
}

void type_free(void *t) {
	free(t);
}

struct var * var_new(const char *name)
{
	struct var * v = malloc(sizeof *v);

	if (v == NULL) return NULL;

	v->t = stack_new();
	v->vn = strdup(name);

	v->tt = UND_T;
	struct type *tmp = type_new(UND_T);
	/* Constants start with a capital letter */
	if (name[0] > 'A' && name[0] < 'Z') {
		tmp->tc = 1;
	} else {
		tmp->tc = 0;
	}
	stack_push(v->t, tmp);

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
		stack_free(&v->t, type_free);
		f->t = NULL;
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

struct cst * cst_new(char type, char cst_type)
{
	struct cst *c = malloc(sizeof *c);

	c->type = type;
	c->reg  = (cst_type == CST_PURECST) ? 0 : new_reg();

	return c;
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
		case STR_T:
			printf("INT: %s\n", c->s);
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
	f->instr = stack_new();

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

	if (f->instr != NULL) {
		// TODO replace NULL by a fonction that free memory correctly
		stack_free(&f->instr, NULL);
		f->instr = NULL;
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
