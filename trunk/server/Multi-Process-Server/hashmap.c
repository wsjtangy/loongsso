#include <math.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "hashmap.h"


static const unsigned int sizes[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457, 1610612741
};

static const int sizes_count   = sizeof(sizes) / sizeof(sizes[0]);
static const float load_factor = 0.65;

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
    int fd;
	struct hashmap *h;
	
	h  = mmap(0, sizeof(struct hashmap) + sizeof(struct record) * capacity, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(h == MAP_FAILED) 
	{
        printf("mmap error(%s)\n", strerror(errno));
		close(fd);
        exit(0);
	}

	h->records       = (struct record *)(((char *)h) + sizeof(struct hashmap));
    h->records_count = 0;
    h->size_index    = capacity;

    return h;
}

void hashmap_destroy(hashmap *h)
{
	munmap(h, sizeof(struct hashmap) + sizeof(struct record) * h->size_index);
}

int hashmap_add(hashmap *h, char *path, unsigned int length, time_t file_time)
{
	int rc;
    struct record *recs;
    unsigned int off, ind, size, code;

    if (path == NULL || *path == '\0') return -2;

    code = strhash(path);
    recs = h->records;
    size = h->size_index;

    ind  = code % size;
    off  = 0;
	
//	printf("size = %u\tcode = %u\tind = %u\r\n", size, code, ind);
    while (recs[ind].hash)
	{
//		printf("ind = %u\r\n", ind);
		if ((code == recs[ind].hash) && strcmp(path, recs[ind].path) == 0) return 0;
        ind = (code + (int)pow(++off,2)) % size;
	}
	
	recs[ind].hash       = code;
	recs[ind].visit      = 1;
	recs[ind].length     = length;
	recs[ind].visit_time = time((time_t*)0);
	recs[ind].file_time  = file_time;
	
	memset(&recs[ind].path, 0, sizeof(recs[ind].path));
	memcpy(recs[ind].path, path, sizeof(recs[ind].path));
    
	h->records_count++;

    return 1;
}

const struct record *hashmap_get(hashmap *h, const char *path)
{
    struct record *recs;
    unsigned int off, ind, size;
    unsigned int code = strhash(path);

    recs = h->records;
    size = h->size_index;
    ind  = code % size;
    off  = 0;

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
    size = h->size_index;
    ind  = code % size;
    off  = 0;

    while (recs[ind].hash) 
	{
        if ((code == recs[ind].hash) && strcmp(path, recs[ind].path) == 0)
		{
            // do not erase hash, so probes for collisions succeed
            recs[ind].hash   = 0;
            recs[ind].visit  = 0;
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
int main()
{
	pid_t pid;
	int fd, rc;
	hashmap *mc;
	struct stat sbuf;
	const struct record *recs;
	char filename[] = "./Hump.mp3";

	mc  = hashmap_new(769);

	pid = fork();
	if (pid == 0) 
	{
		sleep(2);

		recs = hashmap_get(mc, filename);
		if(recs)
		{
			printf("size = %u\tfile_time = %u\tpid = %lu\r\n", recs->length, recs->file_time, getpid());
		}
		else
		{
			printf("ц╩сп\r\n");
		}

		sleep(1);
		_exit(EXIT_SUCCESS);
	}

	fd  = open(filename, O_RDONLY);
	if(fd == -1) return 0;
	
	fstat(fd, &sbuf);
	
	close(fd);
	
//	rc = hashmap_add(mc, filename, sbuf.st_size, sbuf.st_mtime);
	
	printf("rc = %d\tpid = %lu\r\n", rc, getpid());
//	hashmap_remove(memdish, "./sock_epoll.c");

	sleep(5);	
	hashmap_destroy(mc);
	return 1;
}
*/

