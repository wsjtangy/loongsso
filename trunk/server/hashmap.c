#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "server.h"

struct record 
{
    char    path[200];   //文件路径
	uint64_t length;     //文件内容的大小
	unsigned int hash;
};

struct hashmap 
{
    struct record *records;
    unsigned int records_count;
    unsigned int size_index;
};

static const unsigned int sizes[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

static const int sizes_count   = sizeof(sizes) / sizeof(sizes[0]);
static const float load_factor = 0.65;


static int hashmap_grow(hashmap *h)
{
    int i;
    struct record *old_recs;
    unsigned int old_recs_length;

    old_recs_length = sizes[h->size_index];
    old_recs = h->records;

    if (h->size_index == sizes_count - 1) return -1;
    if ((h->records = calloc(sizes[++h->size_index], sizeof(struct record))) == NULL) 
	{
        h->records = old_recs;
        return -1;
    }

    h->records_count = 0;

    // rehash table
    for (i=0; i < old_recs_length; i++)
	{
        if (old_recs[i].hash)
		{
            hashmap_add(h, old_recs[i].path, old_recs[i].length);
		}
	}
    free(old_recs);

    return 0;
}


static unsigned int strhash(const char *str)
{
    int c;
    int hash = 5381;
    while (c = *str++)
        hash = hash * 33 + c;
    return hash == 0 ? 1 : hash;
}


hashmap *hashmap_new(unsigned int capacity) 
{
    int i, sind;
	struct hashmap *h;

    capacity /= load_factor;

    for (i=0; i < sizes_count; i++) 
        if (sizes[i] > capacity) { sind = i; break; }

    if ((h = malloc(sizeof(struct hashmap))) == NULL) return NULL;
    if ((h->records = calloc(sizes[sind], sizeof(struct record))) == NULL) 
	{
        free(h);
        return NULL;
    }

    h->records_count = 0;
    h->size_index    = sind;

    return h;
}

void hashmap_destroy(hashmap *h)
{
    safe_free(h->records);
    safe_free(h);
}

int hashmap_add(hashmap *h, char *path, unsigned int length)
{
	int rc;
    struct record *recs;
    unsigned int off, ind, size, code;

    if (path == NULL || *path == '\0') return -2;
    if (h->records_count > sizes[h->size_index] * load_factor) 
	{
        rc = hashmap_grow(h);
        if (rc) return rc;
    }

    code = strhash(path);
    recs = h->records;
    size = sizes[h->size_index];

    ind = code % size;
    off = 0;

    while (recs[ind].hash)
	{
		if ((code == recs[ind].hash) && strcmp(path, recs[ind].path) == 0) return 0;
        ind = (code + (int)pow(++off,2)) % size;
	}
	
	recs[ind].hash       = code;
	recs[ind].length     = length;
	
	memset(&recs[ind].path, 0, sizeof(recs[ind].path));
	memcpy(recs[ind].path, path, sizeof(recs[ind].path));

	h->records_count++;

    return 1;
}

uint64_t hashmap_get(hashmap *h, const char *path)
{
    struct record *recs;
    unsigned int off, ind, size;
    unsigned int code = strhash(path);

    recs = h->records;
    size = sizes[h->size_index];
    ind = code % size;
    off = 0;

    // search on hash which remains even if a record has been removed,
    // so hash_remove() does not need to move any collision records
    while (recs[ind].hash) 
	{
        if ((code == recs[ind].hash) && strcmp(path, recs[ind].path) == 0)
        {
			return recs[ind].length;
        }
		ind = (code + (int)pow(++off,2)) % size;
    }

    return 0;
}


int hashmap_remove(hashmap *h, const char *path)
{
    struct record *recs;
    unsigned int off, ind, size;
	unsigned int code = strhash(path);

    recs = h->records;
    size = sizes[h->size_index];
    ind = code % size;
    off = 0;

    while (recs[ind].hash) 
	{
        if ((code == recs[ind].hash) && strcmp(path, recs[ind].path) == 0)
		{
            // do not erase hash, so probes for collisions succeed
            recs[ind].hash   = 0;
			recs[ind].length = 0;
			
            h->records_count--;
            return 1;
        }
        ind = (code + (int)pow(++off, 2)) % size;
    }
 
    return 0;
}

unsigned int hashmap_size(hashmap *h)
{
    return h->records_count;
}

/*

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hashmap.h"


int main()
{
	hashmap *memdisk;
	unsigned char *buf;
	unsigned int length;
	
	memdisk = hashmap_new(100);
	
	hashmap_add(memdisk, "./sock_epoll.c", 123);

	hashmap_remove(memdisk, "./sock_epoll.c");
	length = hashmap_get(memdisk, "./sock_epoll.c");
	if(length)
	{
		printf("size = %u\r\n", length);
	}
	
	hashmap_destroy(memdisk);
	return 1;
}

*/
