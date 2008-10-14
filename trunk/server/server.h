#ifndef SERVER_H
#define SERVER_H

#include <time.h>
#include <sys/time.h>
#include "sock_epoll.h"

#define  MAX_FD        4096
#define  SOCK_TIMEOUT  30
#define  safe_free(x)  if(x){free(x);x=NULL;}

typedef void PF(int fd);

struct conn_t 
{
	int     fd;
	time_t  now;
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
