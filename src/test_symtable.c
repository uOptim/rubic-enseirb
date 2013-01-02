#include "symtable.h"
#include <stdio.h>

void dump_type_val (void) {
	printf("FUN_T: %d\t",   FUN_T);
	printf("CLA_T: %d\t",   CLA_T);
	printf("OBJ_T: %d\t",   OBJ_T);
	printf("INT_T: %d\t",   INT_T);
	printf("FLO_T: %d\t",   FLO_T);
	printf("STR_T: %d\n\n", STR_T);
}

void dump_class(class_t * c) { 
	printf("class\n");
	printf("\tclass name: %s\n", c->name);
}

void dump_obj(object_t * o) { 
	printf("object\n");
	printf("\tclass name: %s\n", o->cn);
}

void dump_func(function_t * f) { 
	printf("function\n");
	printf("\treturn type: %d\n", f->ret);
	if (f->ret == OBJ_T) {
		printf("\treturned object class name: %s\n", f->cn);
	}
}

void dump_type(type_t * type) {
	printf("type: %d\t is const: %d\n", type->tt, type->tc);
	if (type->t == CLA_T) {
		dump_class(&type->c);
	}
	else if (type->t == OBJ_T) {
		dump_obj(&type->o);
	}
	else if (type->t == FUN_T) {
		dump_func(&type->f);
	}
}

int main (void) {
	type_t x_type;      /* float */
	type_t a_type;      /* entier */
	type_t s_type;      /* chaîne de caractères */
	type_t A_type;      /* classe */
	type_t o_type;      /* objet */
	type_t f_type;      /* fonction*/
	type_t MYCONST_type;/* chaîne de caractères constante */

	x_type.t = FLO_T;
	a_type.t = INT_T;
	s_type.t = STR_T;
	A_type.t = CLA_T;
	o_type.t = OBJ_T;
	f_type.t = FUN_T;
	MYCONST_type.t = STR_T;
	MYCONST_type.tc = 1;

	A_type.c.name = "A"; 
	o_type.o.cn = "A"; // o est une instance de la classe A
	//f_type.f.name = "f";
	f_type.f.ret = INT_T;

	dump_type_val();
	dump_type(&x_type);
	dump_type(&a_type);
	dump_type(&s_type);
	dump_type(&A_type);
	dump_type(&o_type);
	dump_type(&f_type);
	dump_type(&MYCONST_type);

	return 0;
}
