#ifndef BLOCK_H
#define BLOCK_H

#include "hashmap.h"


struct block {
	struct hashmap *classes;
	struct hashmap *variables;
	struct hashmap *functions;
};

struct block * block_new();
void           block_free(void *);

#endif

