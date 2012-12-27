#include "symtable.h"
#include <stdio.h>

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

void dump_type(void * t) {
	type_t * type = (type_t *)t;
	if (type->tt < 0 || type->tt >= NB_T) {
		printf("Invalid type.\n");
	}
	else {
		printf("type: %d (%s)\t is const: %d\n",
				type->tt, tt[type->tt], type->tc);
		if (type->tt == CLA_T) {
			dump_class(&type->c);
		}
		else if (type->tt == OBJ_T) {
			dump_obj(&type->o);
		}
		else if (type->tt == FUN_T) {
			dump_func(&type->f);
		}
	}
}
