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
	void **array;
};


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

size_t hash(const char *key)
{
	size_t n = 0;
	const char *p = key;

	for (p; *p != '\0'; p++) {
		n += *p;
	}

	return n % SIZE;
}


struct hashmap * hashmap_new() 
{
	struct hashmap *h = calloc(SIZE, sizeof (void *));

	if (h == NULL)
		return NULL;

	h->array = (void *) h;

	if (h == NULL) {
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

		cur = cur->next;
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

