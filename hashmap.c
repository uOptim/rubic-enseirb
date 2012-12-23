#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashmap.h"


#define SIZE 512


struct cell {
	void *data;
	const char *key;
	struct cell *next;
};

struct hashmap {
	struct cell **array;
};



size_t hash(const char *key)
{
	size_t n = 0;
	const char *p = key;

	for (p; *p != '\0'; p++) {
		n += *p;
	}

	return n % SIZE;
}


struct cell * cell_new(const char *k, void *data)
{
	struct cell *c = malloc(sizeof *c);

	if (c == NULL)
		return NULL;

	c->key = strdup(k);

	c->data = data;
	c->next = NULL;

	return c;
}


void hashmap_free(struct hashmap **h)
{
	unsigned int idx;
	struct cell *cur;

	for (idx = 0; idx < SIZE; idx++) {
		cur = (*h)->array[idx];
		if (cur != NULL) free(cur);
	}

	free((*h)->array);
	free(*h);

	*h = NULL;
}


struct hashmap * hashmap_new() 
{
	struct hashmap *h = malloc(sizeof *h);
	
	if (h == NULL)
		return NULL;

	h->array = calloc(SIZE, sizeof (void *));

	if (h->array == NULL) {
		free(h);
		return NULL;
	}

	return h;
}


void * hashmap_get(struct hashmap *h, const char *k)
{
	size_t idx = hash(k);
	struct cell *cur = h->array[idx];

	for (cur; cur != NULL; cur = cur->next) {
		if (strcmp(cur->key, k) == 0) {
			return cur->data;
		}
	}

	fprintf(stderr, "%s: not in this hash!\n", k);
	return NULL;
}


int hashmap_set(struct hashmap *h, const char *k, void *v)
{
	size_t idx = hash(k);
	struct cell *c = cell_new(k, v);

	if (c == NULL) return -1;

	if (h->array[idx] == NULL) {
		h->array[idx] = c;
	}
	
	else {
		struct cell *cur = h->array[idx];
		while (cur->next != NULL) cur = cur->next;
		cur->next = c;
	}

	return 0;
}

