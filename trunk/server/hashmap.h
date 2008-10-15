#ifndef __HASHMAP_H__
#define __HASHMAP_H__


typedef struct hashmap hashmap;

struct record 
{
    char    path[128];       //文件路径
    void    *content;        //文件内容
	time_t  file_time;       //文件的最后修改时间
	time_t  visit_time;      //最后被发送的时间
	unsigned int length;     //文件内容的大小
	unsigned int visit;      //被发送的次数
	unsigned int hash;
};

struct hashmap 
{
	uint64_t length;
    struct record *records;
    unsigned int records_count;
    unsigned int size_index;
};

void hashmap_destroy(hashmap *h);

uint64_t hashmap_length(hashmap *h);

hashmap *hashmap_new(unsigned int capacity);

int hashmap_remove(hashmap *h, const char *path);

const struct record *hashmap_get(hashmap *h, const char *path);

int hashmap_add(hashmap *h, char *path, void *content, unsigned int length, time_t file_time);

#endif
