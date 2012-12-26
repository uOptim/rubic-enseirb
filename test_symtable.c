#include "symtable.h"
#include <stdio.h>

void dump_type_val (void) {
	printf("FUN: %d\t", FUN);
	printf("CLA: %d\t", CLA);
	printf("OBJ: %d\t", OBJ);
	printf("INT: %d\t", INT);
	printf("FLO: %d\t", FLO);
	printf("PTR: %d\t", PTR);
	printf("STR: %d\n\n", STR);
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
	if (f->ret == OBJ) {
		printf("\treturned object class name: %s\n", f->cn);
	}
}

void dump_type(type_t * type) {
	printf("type: %d\t is const: %d\n", type->tt, type->tc);
	if (type->t == CLA) {
		dump_class(&type->c);
	}
	else if (type->t == OBJ) {
		dump_obj(&type->o);
	}
	else if (type->t == FUN) {
		dump_func(&type->f);
	}
}

int main (void) {
	type_t x_type;      /* float */
	type_t p_type;  	/* pointeur */
	type_t a_type;      /* entier */
	type_t s_type;      /* chaîne de caractères */
	type_t A_type;      /* classe */
	type_t o_type;      /* objet */
	type_t f_type;      /* fonction*/
	type_t MYCONST_type;/* chaîne de caractères constante */

	x_type.t = FLO;
	p_type.t = PTR;
	a_type.t = INT;
	s_type.t = STR;
	A_type.t = CLA;
	o_type.t = OBJ;
	f_type.t = FUN;
	MYCONST_type.t = STR;
	MYCONST_type.tc = 1;

	A_type.c.name = "A"; 
	o_type.o.cn = "A"; // o est une instance de la classe A
	//f_type.f.name = "f";
	f_type.f.ret = INT;

	dump_type_val();
	dump_type(&x_type);
	dump_type(&p_type);
	dump_type(&a_type);
	dump_type(&s_type);
	dump_type(&A_type);
	dump_type(&o_type);
	dump_type(&f_type);
	dump_type(&MYCONST_type);

	return 0;
}
