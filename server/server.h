#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "sock_epoll.h"

#define  MAX_FD        4096
#define  SOCK_TIMEOUT  30
#define  safe_free(x)  if(x){free(x);x=NULL;}

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
	int fd;
	int keep_alive;
	
	char uri[50];
	char filepath[300];
	char query_ptr[256];
	
	struct stat    filestats;
	unsigned int   status_code;
	http_version_t http_version;
	http_method_t  http_method;
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
	struct conn_t   *conn;
};

// global vars 

struct sock_epoll_t ct;

struct server_t server;


// funcs 
void http_request_read(int fd);

void http_request_write(int fd);

void http_request_accept(int fd);

#endif
