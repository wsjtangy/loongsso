#ifndef __EVIO_EPOLL__
#define __EVIO_EPOLL__

#include <sys/epoll.h>

#define EVIO_IN	    EPOLLIN
#define EVIO_ET	    EPOLLET
#define EVIO_ERR	EPOLLERR
#define EVIO_OUT    EPOLLOUT 


struct sock_epoll_t
{
	int                 efd;
	unsigned int        max_events;
	struct epoll_event  *events;
};


void sock_epoll_free();

int sock_epoll_del(int fd);

int sock_epoll_wait(int timeout);

int sock_epoll_add(int fd, int events);

int sock_epoll_mod(int fd, int events);

int sock_epoll_init();

#endif

