#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "server.h"


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
            hashmap_add(h, old_recs[i].path, old_recs[i].content, old_recs[i].length, old_recs[i].file_time);
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

	h->length        = 0;
    h->records_count = 0;
    h->size_index    = sind;

    return h;
}

void hashmap_destroy(hashmap *h)
{
	int i;
	struct record *recs;
	unsigned int  recs_length;
	
	recs        = h->records;
    recs_length = sizes[h->size_index];
    
    for (i=0; i < recs_length; i++)
	{
        if (recs[i].hash) safe_free(recs[i].content);
	}

    safe_free(h->records);
    safe_free(h);
}

int hashmap_add(hashmap *h, char *path, void *content, unsigned int length, time_t file_time)
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
            recs[ind].hash   = 0;
            recs[ind].visit  = 0;

			h->length       -= recs[ind].length;
			recs[ind].length = 0;
			
			safe_free(recs[ind].content);
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
	int fd;
	struct stat sbuf;
	hashmap *memdish;
	unsigned char *buf;
	const struct record *recs;
	
	memdish = hashmap_new(100);
	fd      = open("./sock_epoll.c", O_RDONLY);
	if(fd == -1) return 0;
	
	fstat(fd, &sbuf);
	
	buf = calloc(sbuf.st_size + 1, sizeof(unsigned char));

	read(fd, buf, sbuf.st_size);
	close(fd);

	//printf("%s\r\nsize = %u\r\n", buf, sbuf.st_size);
	
	hashmap_add(memdish, "./sock_epoll.c", buf, sbuf.st_size, sbuf.st_mtime);

	hashmap_remove(memdish, "./sock_epoll.c");
	recs = hashmap_get(memdish, "./sock_epoll.c");
	if(recs)
	{
		printf("%s\r\nsize = %u\r\n", recs->content, recs->length);
	}

	return 1;
}
*/

