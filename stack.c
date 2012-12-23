#include <stdio.h>
#include <stdlib.h>

#include "stack.h"


struct elt {
	void *data;
	struct elt *next;
};


struct stack {
	struct elt *head;
};
	

struct stack * stack_new()
{
	struct stack *s = malloc(sizeof *s);

	if (s == NULL) {
		perror("malloc");
	} else {
		s->head = NULL;
	}

	return s;
}


void stack_free(struct stack **s)
{
	free(*s);
}


void * stack_pop(struct stack *s)
{
	void *data = NULL;
	struct elt *e = s->head;
	
	if (e != NULL) {
		data = e->data;
		s->head = e->next;
		free(e);
	}

	return data;
}


int stack_push(struct stack *s, void *d)
{
	struct elt *e = malloc(sizeof *e);

	if (e == NULL) {
		perror("malloc");
		return -1;
	}

	e->data = d;
	e->next = s->head;
	s->head = e;

	return 0;
}
