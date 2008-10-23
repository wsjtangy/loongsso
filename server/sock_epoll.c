#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include "server.h"


int sock_epoll_init()
{
	ct.max_events  = MAX_FD;
	ct.efd = epoll_create(MAX_FD);
	if (ct.efd == -1) 
	{
		perror("initializing poll queue");
		return 0;
	}
	
	ct.events = calloc(ct.max_events, sizeof(struct epoll_event));
	if(ct.events == NULL) return 0;

	return 1;
}


void sock_epoll_free()
{
	close(ct.efd);
	close(server.listen_fd);
	safe_free(ct.events);
}

int sock_epoll_ctl(int func, int fd, int events) 
{
	struct epoll_event ev;
	
	ev.events   = events;
	ev.data.fd  = fd;

	return epoll_ctl(ct.efd, func, fd, &ev);
}

int sock_epoll_add(int fd, int events) 
{
	return sock_epoll_ctl(EPOLL_CTL_ADD, fd, events);
}

int sock_epoll_mod(int fd, int events) 
{
	return sock_epoll_ctl(EPOLL_CTL_MOD, fd, events);
}

int sock_epoll_del(int fd) 
{
	return sock_epoll_ctl(EPOLL_CTL_DEL, fd, 0);
}

int sock_epoll_wait(int timeout) 
{
	int n, i;
	time_t epoll_time;
	struct epoll_event *cevents;

	for( ; ; )
    {
		n = epoll_wait(ct.efd, ct.events, ct.max_events, timeout);
		if(n <= 0) continue;
		
		/*
		// check timeout 
		epoll_time = time((time_t*)0);
		for(i = 0; i <= server.maxfd; i++)
		{
			struct conn_t *c = &server.conn[i];
			if(c->fd &&  (c->fd != server.listen_fd) && (epoll_time > (c->now + SOCK_TIMEOUT)))
			{
				sock_close(c->fd);
			}
		}
		*/

		for (i = 0, cevents = ct.events; i < n; i++, cevents++) 
		{	
			if(cevents->events & (EPOLLHUP | EPOLLERR) && cevents->data.fd != server.listen_fd)
			{
				sock_close(cevents->data.fd);
			}
			else if(cevents->events & EPOLLIN && cevents->data.fd == server.listen_fd)
			{
				http_request_accept(cevents->data.fd);
			}
			else if(cevents->events & EPOLLIN)
			{
				http_request_read(cevents->data.fd);
			}
			else if(cevents->events & EPOLLOUT)
			{
				http_request_write(cevents->data.fd);
			}
		}
    }
}
