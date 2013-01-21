#ifndef STACK_H
#define STACK_H

struct stack;


struct stack * stack_new();
void stack_free(struct stack **, void (*)(void *));

void * stack_pop(struct stack *);
void * stack_peak(struct stack *, unsigned int);
int    stack_push(struct stack *, void *);
int    stack_size(struct stack *s);


#endif
