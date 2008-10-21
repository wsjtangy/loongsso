#ifndef __HASHMAP_H__
#define __HASHMAP_H__


typedef struct hashmap hashmap;

void hashmap_destroy(hashmap *h);

unsigned int hashmap_size(hashmap *h);

hashmap *hashmap_new(unsigned int capacity);

int hashmap_remove(hashmap *h, const char *path);

int hashmap_add(hashmap *h, char *path, unsigned int length);

uint64_t hashmap_get(hashmap *h, const char *path);

#endif
