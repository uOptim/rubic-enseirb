#ifndef STACK_H
#define STACK_H

struct stack;


struct stack * stack_new();
void stack_free(struct stack **);

void * stack_pop(struct stack *);
int    stack_push(struct stack *, void *);


#endif
