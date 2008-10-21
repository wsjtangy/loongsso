#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>  
#include "server.h"
#include "sock_io.h"

int sock_set_options(int fd)
{
	int flags = 1;

	if(fd <= 0) return -1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(int)) == -1) 
	{
		return -1;
	}

	flags = 0;
	if ( (flags = fcntl(fd, F_GETFL, 0)) == -1) 
	{
		return -1;
	}

	if ((fcntl(fd, F_SETFL, flags | O_NONBLOCK)) == -1) 
	{
		return -1;
	}

	return fd;
}


int sock_set_noblocking(int fd) 
{
	int flag;
	int dummy = 0;
	
	if(-1 == (flag = fcntl(fd, F_GETFL, dummy))) 
	{
		//log_debug(__FILE__, __LINE__, "%s. fd %d\n", strerror(errno), fd);
		return -1;
	}

	if(-1 == (flag = fcntl(fd, F_SETFL, dummy | O_NONBLOCK | O_NDELAY))) 
	{
		//log_debug(__FILE__, __LINE__, "%s. fd %d\n", strerror(errno), fd);
		return -1;
	}

	return 0;
}

void sock_init() 
{
	server.port      = 7878;
	server.maxfd     = 0;
	server.listen_fd = -1;
	server.root      = "/home/new_you54_user/server/www";
	server.conn      = calloc(MAX_FD, sizeof(struct conn_t));
	
	sock_epoll_init();
	
	server.listen_fd = sock_listen_open();

	sock_epoll_add(server.listen_fd, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP);
	
}

void sock_close(int fd) 
{	
	struct conn_t *c = &server.conn[fd];
	if(!c->fd)
		return;
	
	sock_epoll_del(fd);
	fd_close(fd);
}

		
void fd_open(int fd)
{
	struct conn_t *c = &server.conn[fd];

	c->fd  = fd;
	c->now = time((time_t*)0);
	if(fd > server.maxfd)
	{
		server.maxfd = fd;
	}
	
	sock_set_options(fd);
}

static void fd_close(int fd)
{
	struct conn_t *c = &server.conn[fd];

	c->fd = 0;

	if(fd >= server.maxfd) 
	{
		server.maxfd--;
	}

	close(fd);
}

void sock_set_linger(int fd)
{
	struct linger ling;	
	
	ling.l_onoff  = 1;
	ling.l_linger = 10;
	if(-1 == setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling))) 
	{
		//log_debug(__FILE__, __LINE__, "warning! sock_set_cork(%s)\n", strerror(errno));
	}
}

static void sock_set_cork(int fd)
{
	int on = 1;
	
	if(-1 == setsockopt(fd, IPPROTO_TCP, TCP_CORK, (char *)&on, sizeof(on))) 
	{
		//log_debug(__FILE__, __LINE__, "warning! sock_set_cork(%s)\n", strerror(errno));
	}
}

static void sock_set_reuseaddr(int fd)
{
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
	{
		//log_debug(__FILE__, __LINE__, "%s. FD %d\n", strerror(errno), fd);
	}
}

static int sock_listen_open()
{
	struct sockaddr_in srv;
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd == -1) 
	{
		printf("sock_listen_open %s\n", strerror(errno));
		exit(0);
	}
	
	fd_open(sd);

	srv.sin_family        = AF_INET;
	srv.sin_port          = htons(server.port);
	srv.sin_addr.s_addr   = htonl(INADDR_ANY);

	
	
	if(-1 == bind(sd, (struct sockaddr *) &srv, sizeof(srv))) 
	{
		printf("sock_listen_open %s\n", strerror(errno));
		exit(0);
	}

	listen(sd, MAX_FD);

	return sd;
}

