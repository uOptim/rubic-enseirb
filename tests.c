#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "stack.h"
#include "hashmap.h"


void stack_t()
{
	struct stack *s = stack_new();

	stack_push(s, "schtroumpf");
	stack_push(s, "bidule");
	stack_push(s, "truc");
	stack_push(s, "machin");

	assert(0 == strcmp("schtroumpf", (char *) stack_peak(s, 3)));
	assert(NULL == stack_peak(s, 4));
	assert(0 == strcmp("machin", (char *) stack_pop(s)));
	assert(0 == strcmp("truc", (char *) stack_pop(s)));

	stack_free(&s, NULL);

	assert(s == NULL);

	fprintf(stderr, "stack_t: OK\n");
}


void hashmap_t()
{
	struct hashmap *h = hashmap_new();

	hashmap_set(h, "toto", "truc");
	hashmap_set(h, "otto", "machin");

	assert(0 == strcmp("truc",   (char *) hashmap_get(h, "toto")));
	assert(0 == strcmp("machin", (char *) hashmap_get(h, "otto")));

	hashmap_free(&h, NULL);

	assert(h == NULL);

	fprintf(stderr, "hashmap_t: OK\n");
}


int main()
{
	stack_t();
	hashmap_t();

	return 0;
}

