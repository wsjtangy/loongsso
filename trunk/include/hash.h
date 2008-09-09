#ifndef __HASH__
#define __HASH__

typedef struct hash hash;

/* Create new hashtable. */
hash * hash_new(unsigned int size);

/* Free hashtable. */
void hash_destroy(hash *h);

/* Add key/value pair. Returns non-zero value on error (eg out-of-memory). */
int hash_add(hash *h, uint64_t key, char *value, time_t lifetime);

/* Return value matching given key. */
const char *hash_get(hash *h, const uint64_t key);

/* Remove key from table, returning value. */
int hash_remove(hash *h, const uint64_t key);

/* Returns total number of keys in the hashtable. */
unsigned int hash_size(hash *h);


#endif
