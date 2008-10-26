#include <math.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mps.h"

//gcc -O2 -D_REENTRANT -D_THREAD_SAFE  -o hashmap hashmap.c libsharedmem.a -lpthread -lrt -lm

static const unsigned int sizes[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

static const int sizes_count   = sizeof(sizes) / sizeof(sizes[0]);
static const float load_factor = 0.65;


static int hashmap_grow(hashmap *h)
{
    int  i;
	long off;
    struct record *old_recs;
    unsigned int old_recs_length;

    old_recs        = h->records;
	old_recs_length = sizes[h->size_index];

    if (h->size_index == sizes_count - 1) return 0;
    
	off = sharedmem_alloc(&shmalloc, sizeof(struct record) * sizes[++h->size_index]);
	if(off == -1)
	{
		h->records = old_recs;
		return 0;
	}
	
	h->records_count = 0;
	h->records       = (struct record *)sharedmem_alloc_get_ptr(&shmalloc, off);

    // rehash table
    for (i=0; i < old_recs_length; i++)
	{
        if (old_recs[i].hash)
		{
            hashmap_add(h, old_recs[i].path, old_recs[i].content, old_recs[i].length, old_recs[i].file_time, old_recs[i].off);
		}
	}

	if(sharedmem_free(&shmalloc, h->c) != 0)
	{
		printf("sharedmem_free(): %s\n", sharedmem_get_error_string(&shm));
	}

	h->c = off;
    return 1;
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
	long p, c;
    int i, sind;
	struct hashmap *h;

    capacity /= load_factor;

    for (i=0; i < sizes_count; i++) 
    {
		if (sizes[i] > capacity) { sind = i; break; }
	}

	p = sharedmem_alloc(&shmalloc, sizeof(struct hashmap));
	if(p == -1) return NULL;
    
	c = sharedmem_alloc(&shmalloc, sizeof(struct record) * sizes[sind]);
	if(c == -1) return NULL;
	

	h          = (struct hashmap *)sharedmem_alloc_get_ptr(&shmalloc, p);
	h->records = (struct record *)sharedmem_alloc_get_ptr(&shmalloc, c);
	
	h->p             = p;
	h->c             = c;
	h->length        = 0;
    h->records_count = 0;
    h->size_index    = sind;

    return h;
}

void hashmap_destroy(hashmap *h)
{
	int i;
	long p, c, off;
	struct record *recs;
    unsigned int  recs_length;
	
	p = h->p;
	c = h->c;

    recs        = h->records;
	recs_length = sizes[h->size_index];
    
	for (i=0; i < recs_length; i++)
	{
        if (recs[i].hash)
		{
			off = recs[i].off;
			if(sharedmem_free(&shmalloc, off) != 0)
			{
				printf("sharedmem_free(): %s\n", sharedmem_get_error_string(&shm));
			}
		}
	}
	
	if(sharedmem_free(&shmalloc, c) != 0)
    {
        printf("sharedmem_free(): %s\n", sharedmem_get_error_string(&shm));
    }
	if(sharedmem_free(&shmalloc, p) != 0)
    {
        printf("sharedmem_free(): %s\n", sharedmem_get_error_string(&shm));
    }
}


int hashmap_add(hashmap *h, const char *path, void *content, unsigned int length, time_t file_time, long offest)
{
	int rc;
    struct record *recs;
    unsigned int off, ind, size, code;

    if (path == NULL || *path == '\0') return -2;
    if (h->records_count > sizes[h->size_index] * load_factor) 
	{
        rc = hashmap_grow(h);
        if (!rc) return rc;
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
	
	printf("hashmap_add ind = %d\tpath = %s\r\n", ind, path);

	recs[ind].off        = offest;
	recs[ind].hash       = code;
	recs[ind].visit      = 1;
	recs[ind].content    = content;
	recs[ind].length     = length;
	recs[ind].visit_time = time((time_t*)0);
	recs[ind].file_time  = file_time;
	
	memset(&recs[ind].path, 0, sizeof(recs[ind].path));
	memcpy(recs[ind].path, path, sizeof(recs[ind].path));
    
	h->length += length;
	h->records_count++;

    return 1;
}


const struct record *hashmap_get(hashmap *h, const char *path)
{
    struct record *recs;
    unsigned int off, ind, size;
    unsigned int code = strhash(path);

	off  = 0;
    recs = h->records;
    size = sizes[h->size_index];
	ind  = code % size;

    while (recs[ind].hash)
	{
        if ((code == recs[ind].hash) && strcmp(path, recs[ind].path) == 0)
        {
			recs[ind].visit++;
			recs[ind].visit_time = time((time_t*)0);
			
			return &recs[ind];
        }
		ind = (code + (int)pow(++off,2)) % size;
    }

    return NULL;
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
            
			if(sharedmem_free(&shmalloc, recs[ind].off) != 0)
			{
				printf("sharedmem_free(): %s\n", sharedmem_get_error_string(&shm));
			}

			recs[ind].off    = 0;
			recs[ind].hash   = 0;
            recs[ind].visit  = 0;

			h->length       -= recs[ind].length;
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

uint64_t hashmap_length(hashmap *h)
{
    return h->length;
}



