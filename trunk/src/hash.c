#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "hash.h"

//ident.c
static const unsigned int sizes[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};


static const int   sizes_count   = sizeof(sizes) / sizeof(sizes[0]);
static const int   code_time_out = 2;
static const float load_factor = 0.65;

struct record 
{
	char      value[5];
	time_t    lifetime;
	uint64_t  key;
};

struct hash 
{
    struct record  *records;
    unsigned int   size_index;
	unsigned int   records_count;
};


int _timeout(time_t t1)
{
	int n;
	time_t now;

	now = time((time_t*)0);
	n   = (int)difftime(now, t1);
	n  /= 60;   //转换为分钟
	
	if(n >= 2)
	{
		//超时,返回假
		return 0;
	}
	return 1;
}


static int hash_grow(hash *h)
{
    int i, rc;
    struct record *old_recs;
    unsigned int old_recs_length;

    old_recs_length = sizes[h->size_index];
    old_recs        = h->records;

    if (h->size_index == sizes_count - 1) return -1;
    if ((h->records = calloc(sizes[++h->size_index],sizeof(struct record))) == NULL)
	{
        h->records = old_recs;
        return -1;
    }
	
    h->records_count = 0;

    // rehash table
    for (i=0; i < old_recs_length; i++)
	{
        if (old_recs[i].key)
		{
			rc = _timeout(old_recs[i].lifetime);
			if(rc)
			{
				hash_add(h, old_recs[i].key, old_recs[i].value, old_recs[i].lifetime);
			}
		}
	}

    free(old_recs);

    return 0;
}


hash * hash_new(unsigned int capacity) 
{
    struct hash *h;
    int i, sind;

    capacity /= load_factor;

    for (i=0; i < sizes_count; i++) 
	{
        if (sizes[i] > capacity) { sind = i; break; }
	}

    if ((h = malloc(sizeof(struct hash))) == NULL) return NULL;
    if ((h->records = calloc(sizes[sind], sizeof(struct record))) == NULL) 
	{
        free(h);
        return NULL;
    }

    h->records_count = 0;
    h->size_index    = sind;
	
    return h;
}

void hash_destroy(hash *h)
{
    free(h->records);
    free(h);
}

int hash_add(hash *h, uint64_t key, char *value, time_t lifetime)
{
	int rc;
    struct record *recs;
    unsigned int off, ind, size;

    if (key <= 0 ) return -2;	
    if (h->records_count > sizes[h->size_index] * load_factor) 
	{
        rc = hash_grow(h);
        if (rc) return rc;
    }

    recs = h->records;
    size = sizes[h->size_index];

    ind = key % size;
    off = 0;
	rc  = 1;

    while (recs[ind].key)
	{
		rc = _timeout(recs[ind].lifetime);
		if(!rc)
		{
			//超时的记录
            h->records_count--;
			break;
		}

		if(key == recs[ind].key)
		{
			return 0;
		}

        ind = (key + (int)pow(++off,2)) % size;
	}
	
	recs[ind].key       = key;
	recs[ind].lifetime  = lifetime;

	memcpy(recs[ind].value, value, sizeof(recs[ind].value));
	
    h->records_count++;

    return 1;
}


const char *hash_get(hash *h, const uint64_t key)
{
	int rc;
    struct record *recs;
    unsigned int off, ind, size;

    recs = h->records;
    size = sizes[h->size_index];
    ind  = key % size;
    off  = 0;

    while (recs[ind].key) 
	{
		rc = _timeout(recs[ind].lifetime);
		if(!rc)
		{
			//超时的记录,将这条记录置为删除的标志
			recs[ind].key = 0;
            h->records_count--;
			break;
		}
        if (key == recs[ind].key)
		{
            return recs[ind].value;
        }
		ind = (key + (int)pow(++off,2)) % size;
    }

    return NULL;
}

int hash_remove(hash *h, const uint64_t key)
{
    struct record *recs;
    unsigned int off, ind, size;

    recs = h->records;
    size = sizes[h->size_index];
    ind  = key % size;
    off  = 0;

    while (recs[ind].key)
	{
        if (key == recs[ind].key)
		{
			recs[ind].key = 0;
            h->records_count--;
            return 1;
        }
        ind = (key + (int)pow(++off, 2)) % size;
    }
 
    return 0;
}

unsigned int hash_size(hash *h)
{
    return h->records_count;
}

