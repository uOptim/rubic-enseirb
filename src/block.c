#include <stdlib.h>

#include "block.h"
#include "symtable.h"


struct block * block_new()
{
	struct block *b = malloc(sizeof *b);

	b->classes   = hashmap_new();
	b->functions = hashmap_new();
	b->variables = hashmap_new();

	return b;
}


void block_free(void *block)
{
	struct block *b = (struct block *) block;

	hashmap_free(&b->classes, sym_free);
	hashmap_free(&b->functions, sym_free);
	hashmap_free(&b->variables, sym_free);

	free(b);
}
