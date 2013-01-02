#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashmap.h"


#define SIZE 512


struct cell {
	void *data;
	char *key;
	struct cell *next;
};

struct hashmap {
	struct cell **array;
};



static size_t hash(const char *key)
{
	size_t n = 0;
	const char *p = key;

	for (; *p != '\0'; p++) {
		n += *p;
	}

	return n % SIZE;
}


static struct cell * cell_new(const char *k, void *data)
{
	struct cell *c = malloc(sizeof *c);

	if (c == NULL)
		return NULL;

	c->key = strdup(k);

	c->data = data;
	c->next = NULL;

	return c;
}


void hashmap_free(struct hashmap **h, void (*free_data)(void *))
{
	unsigned int idx;
	struct cell *cur, *tmp;

	for (idx = 0; idx < SIZE; idx++) {
		cur = (*h)->array[idx];

		while (cur != NULL) {
			tmp = cur->next;
			free(cur->key);
			if (free_data != NULL) free_data(cur->data);
			free(cur);
			cur = tmp;
		}
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

	h->array = calloc(SIZE, sizeof *h->array);

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

	while(cur != NULL) {
		if (strcmp(cur->key, k) == 0) {
			return cur->data;
		}

		cur = cur->next;
	}

	return NULL;
}


int hashmap_set(struct hashmap *h, const char *k, void *v)
{
	size_t idx = hash(k);
	struct cell *c = cell_new(k, v);

	if (c == NULL) return -1;

	if (h->array[idx] != NULL) {
		c->next = h->array[idx];
	}
	
	h->array[idx] = c;

	return 0;
}

void hashmap_dump(struct hashmap *h, void (*dump_data)(void *))
{
	unsigned int idx;
	struct cell *cur;

	for (idx = 0; idx < SIZE; idx++) {
		cur = h->array[idx];

		while (cur != NULL) {
			printf("Key: %s\n", cur->key);
			dump_data(cur->data);
			cur = cur->next;
		}
	}
}
