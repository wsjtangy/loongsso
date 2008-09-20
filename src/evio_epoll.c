#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <loong.h>


int evio_epoll_init(struct ev_ct *ct, int max_events)
{
	ct->event_count = 0;
	ct->max_events  = max_events;
	
	ct->fd = epoll_create(max_events);
	if (ct->fd == -1) 
	{
		perror("initializing poll queue");
		return 0;
	}
	
	ct->events = calloc(max_events, sizeof(struct epoll_event));
	if(ct->events == NULL)
	{
		return 0;
	}

	return 1;
}


void evio_epoll_free(struct ev_ct *ct)
{
	close(ct->fd);
	free(ct->events);
}

int evio_epoll_ctl(struct ev_ct *ct, int func, int fd, int events, void *data) 
{
	struct epoll_event ev;
	
	ev.events   = events;
	ev.data.ptr = data;

	return epoll_ctl(ct->fd, func, fd, &ev);
}

int evio_epoll_add(struct ev_ct *ct, int fd, int events, void *data) 
{
	return evio_epoll_ctl(ct, EPOLL_CTL_ADD, fd, events, data);
}

int evio_epoll_mod(struct ev_ct *ct, int fd, int events, void *data) 
{
	return evio_epoll_ctl(ct, EPOLL_CTL_MOD, fd, events, data);
}

int evio_epoll_del(struct ev_ct *ct, int fd) 
{
	return evio_epoll_ctl(ct, EPOLL_CTL_DEL, fd, 0, NULL);
}

int evio_epoll_wait(struct ev_ct *ct, int timeout, int fd, evio_call_accept _accept, evio_call_client _client) 
{
	int n, i;

	for( ; ; )
    {
		n = epoll_wait(ct->fd, ct->events, ct->max_events, timeout);
		if (n == -1 )
		{
			perror("epoll wait error");
		}
		
        for (i = 0; i < n; i++)
        {
			int sfd = *(int *)ct->events[i].data.ptr;
			if (sfd == fd)
            {
				_accept(sfd);
            }
            else
            {
				_client(ct->events[i].data.ptr);
				loong_conn_exit((loong_conn *)ct->events[i].data.ptr);
            }
        }
    }
}

struct ev_method em = {
	.name   = "epoll",
	.free   = evio_epoll_free,
	.init   = evio_epoll_init,
	.wait   = evio_epoll_wait,
	.add    = evio_epoll_add,
	.mod    = evio_epoll_mod,
	.del    = evio_epoll_del,
};
