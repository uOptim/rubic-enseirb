#include <stdio.h>
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

void block_dump(struct block *b)
{
	puts("Dumping block:");
	puts("* variables:");
	hashmap_dump(b->variables, sym_dump);
	puts("* functions:");
	hashmap_dump(b->functions, sym_dump);
	puts("* classes:");
	hashmap_dump(b->classes, sym_dump);
}
