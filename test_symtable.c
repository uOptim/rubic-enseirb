#include "symtable.h"
#include <stdio.h>

void dump_type_val (void) {
	printf("FUN: %d\t", FUN);
	printf("OBJ: %d\t", OBJ);
	printf("CHA: %d\t", CHA);
	printf("INT: %d\t", INT);
	printf("FLO: %d\t", FLO);
	printf("PTR: %d\t", PTR);
	printf("STR: %d\n\n", STR);
}

void dump_class(class_t * class) { 
	printf("class\n");
	printf("\tname: %s\n", class->name);
}

void dump_func(function_t * func) { 
	printf("function\n");
	//printf("\tname: %s\n", func->name);
	printf("\treturn type: %d\n", func->ret);
}

void dump_type(type_t * type) {
	printf("type: %d\t is const: %d\n", type->tt, type->tc);
	if (type->t == OBJ) {
		dump_class(&type->c);
	}
	else if (type->t == FUN) {
		dump_func(&type->f);
	}
}

int main (void) {
	type_t x_type;      /* float */
	type_t p_type;  	/* pointeur */
	type_t c_type;      /* char */
	type_t a_type;      /* entier */
	type_t s_type;      /* chaîne de caractères */
	type_t o_type;      /* objet */
	type_t f_type;      /* fonction*/
	type_t MYCONST_type;/* chaîne de caractères constante */

	x_type.t = FLO;
	p_type.t = PTR;
	c_type.t = CHA;
	a_type.t = INT;
	s_type.t = STR;
	o_type.t = OBJ;
	f_type.t = FUN;
	MYCONST_type.t = STR;
	MYCONST_type.tc = 1;

	o_type.c.name = "A"; // o est une instance de la classe A
	//f_type.f.name = "f";
	f_type.f.ret = INT;

	dump_type_val();
	dump_type(&x_type);
	dump_type(&p_type);
	dump_type(&c_type);
	dump_type(&a_type);
	dump_type(&s_type);
	dump_type(&o_type);
	dump_type(&f_type);
	dump_type(&MYCONST_type);

	return 0;
}
