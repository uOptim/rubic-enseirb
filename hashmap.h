#ifndef HASHMAP_H
#define HASHMAP_H

#include <sys/types.h>


struct hashmap;

struct hashmap * hashmap_new();
void             hashmap_free(struct hashmap **h, void (*free_data)(void *));

void *           hashmap_get(struct hashmap *, const char *);
int              hashmap_set(struct hashmap *, const char *, void *);

void             hashmap_dump(struct hashmap *h, void (*dump_data)(void *));

#endif
