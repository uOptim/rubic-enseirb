#ifndef STACK_H
#define STACK_H

struct stack;


struct stack * stack_new();
void stack_free(struct stack **, void (*)(void *));

void * stack_pop(struct stack *);
void   stack_clear(struct stack *, void (*)(void *));
int    stack_push(struct stack *, void *);
int    stack_size(struct stack *s);
void   stack_move(struct stack *src, struct stack *dst);
void   stack_map(struct stack *, void (*)(void*, void*, void*), void*, void*);

void * stack_next(struct stack *);
void * stack_peak(struct stack *, unsigned int);
void   stack_rewind(struct stack *);
struct stack * stack_copy(struct stack *src, void * (*cpy)(void*));


#endif
