#ifndef __MPS_H
#define __MPS_H

#include <time.h>
#include <sys/time.h>
#include <sys/epoll.h>

#define  MAX_FD        15000
#define  SOCK_TIMEOUT  40
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


struct ev_ct
{
	int                 efd;
	unsigned int        max_events;
	struct epoll_event  *events;
};

struct request
{
	int keep_alive;
	
	char uri[50];
	char filepath[300];
	char query_ptr[256];
	char if_modified_since[50];
	
	unsigned int   status_code;
	http_method_t  http_method;
	http_version_t http_version;

	//send buf
	int          fd;        //file open fd
	unsigned int size;     //已经发送多少字节的数据
	unsigned int length;   //数据的大小
};


struct conn_t 
{
	int             fd;
	time_t          now;
	struct request  req;
};

struct mps_process
{
	pid_t  pid;
	int    channel[2];
};

struct server_t 
{
	int                 maxfd;
	char                *root;
	struct ev_ct        ct;
	struct conn_t       *conn;
	struct mps_process  *proc;
};

// global vars 


// funcs 



#endif
