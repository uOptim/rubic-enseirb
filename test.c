#include <stdio.h>

#include "stack.h"
#include "hashmap.h"


int main()
{
	struct stack *s = stack_new();
	struct hashmap *h = hashmap_new();

	hashmap_set(h, "toto", "truc");
	hashmap_set(h, "otto", "machin");

	printf("toto: %s\n", (char *) hashmap_get(h, "toto"));
	printf("otto: %s\n", (char *) hashmap_get(h, "otto"));

	stack_push(s, "schtroumpf");
	stack_push(s, "bidule");
	stack_push(s, "truc");
	stack_push(s, "machin");

	printf("pop: %s\n", (char *) stack_pop(s));
	printf("pop: %s\n", (char *) stack_pop(s));

	stack_free(s);
	hashmap_free(&h);

	return 0;
}
