#include "symtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NB_T 6

static const char tt[NB_T][4] = {"FUN",
                                 "CLA",
                                 "OBJ",
                                 "INT",
                                 "FLO",
                                 "STR"};

static void dump_class(class_t * c) { 
	printf("class\n");
	printf("\tclass name: %s\n", c->name);
}

static void dump_obj(object_t * o) { 
	printf("object\n");
	printf("\tclass name: %s\n", o->cn);
}

static void dump_func(function_t * f) { 
	printf("function\n");
	printf("\treturn type: %d\n", f->ret);
	if (f->ret == OBJ_T) {
		printf("\treturned object class name: %s\n", f->cn);
	}
}

void type_dump(void * type) {
	type_t * t = (type_t *)type;
	if (t->tt < 0 || t->tt >= NB_T) {
		printf("Invalid type.\n");
	}
	else {
		printf("type: %d (%s)\t is const: %d\n",
				t->tt, tt[t->tt], t->tc);
		if (t->tt == CLA_T) {
			dump_class(&t->c);
		}
		else if (t->tt == OBJ_T) {
			dump_obj(&t->o);
		}
		else if (t->tt == FUN_T) {
			dump_func(&t->f);
		}
	}
}

/* name is used when type is OBJ_T, CLA_T or FUN_T.
 * For a OBJ_T, name is the name of the object class.
 * For a CLA_T, name is the name of the class.
 * For a FUN_T, name is the name of the returned object class when the
 * function returns an object.
 */
type_t * type_new(int type, const char * name) {
	type_t * t = malloc(sizeof *t);

	if (t == NULL)
		return NULL;

	t->t = type;
	if (type == OBJ_T) {
		t->o.cn = strdup(name);
	}
	else if (type == CLA_T) {
		t->c.name = strdup(name);
	}

	return t;
}

void type_free(void * type) {
	type_t **t = (type_t **) type;

	if ((*t)->tt == OBJ_T) {
		free((*t)->o.cn);
	}
	else if ((*t)->tt == CLA_T) {
		free((*t)->c.name);
	}
	
	free(*t);
	*t = NULL;
}
