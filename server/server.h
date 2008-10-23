#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "hashmap.h"
#include "sock_epoll.h"

#define  MAX_FD        15000
#define  SOCK_TIMEOUT  60
#define  safe_free(x)  if(x){free(x);x=NULL;}
#define  RFC_TIME      "%a, %d %b %Y %H:%M:%S GMT"
#define  RFC_304       "HTTP/1.1 %s\r\nServer: Memhttpd/Beta1.0\r\nDate: %s\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"
#define  RFC_404       "HTTP/1.1 %s\r\nServer: Memhttpd/Beta1.0\r\nDate: %s\r\nContent-Type: text/html\r\nContent-Length: %u\r\nConnection: keep-alive\r\n\r\n"

typedef enum 
{
	HTTP_VERSION_1_1,
	HTTP_VERSION_1_0,
	HTTP_VERSION_0_9,
	HTTP_VERSION_UNKNOWN
} http_version_t;

typedef enum 
{
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_UNKNOWN
} http_method_t;


struct request
{
	int keep_alive;
	
	char uri[50];
	char filepath[300];
	char query_ptr[256];
	char if_modified_since[50];
	
	unsigned int   status_code;
	http_version_t http_version;
	http_method_t  http_method;

	//send buf
	void *buf;       //需要发送的数据
	size_t size;     //已经发送多少字节的数据
	size_t length;   //数据的大小
};


struct conn_t 
{
	int             fd;
	time_t          now;
	struct request  req;
};

struct server_t 
{
	int             maxfd;
	int             listen_fd;
	short           port;
	char            *root;
	struct conn_t   *conn;
};

// global vars 

hashmap *memdisk;

struct sock_epoll_t ct;

struct server_t server;

pthread_cond_t   cond;
pthread_mutex_t  mutex;

// funcs 
void http_request_read(int fd);

void http_request_write(int fd);

void http_request_accept(int fd);

#endif
