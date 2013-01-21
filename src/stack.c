#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "stack.h"


struct elt {
	void *data;
	struct elt *next;
};


struct stack {
	struct elt *head;
	int size;
};
	

struct stack * stack_new()
{
	struct stack *s = malloc(sizeof *s);

	if (s == NULL) {
		perror("malloc");
	} else {
		s->head = NULL;
		s->size = 0;
	}

	return s;
}


void stack_free(struct stack **s, void (*free_data)(void *))
{
	struct elt *tmp;
	struct elt *e = (*s)->head;
	while (e != NULL) {
		if (free_data != NULL) free_data(e->data);
		tmp = e->next;
		free(e);
		e = tmp;
	}
	free(*s);
	*s = NULL;
}


void * stack_pop(struct stack *s)
{
	void *data = NULL;
	struct elt *e = s->head;
	
	if (e != NULL) {
		data = e->data;
		s->head = e->next;
		s->size--;
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
	s->size++;

	return 0;
}


void * stack_peak(struct stack *s, unsigned int n)
{
	struct elt *e;
	unsigned int i;

	for (i = 0, e = s->head; i < n && e != NULL; ++i, e = e->next);
	return (e == NULL) ? NULL : e->data;
}

int stack_size(struct stack *s) {
	return s->size;
}
