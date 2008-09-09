#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>
#include <evio_kqueue.h>

int evio_kqueue_init(struct ev_ct *ct, int max_events) 
{
	
	ct->change_count = 0;
	ct->max_events   = max_events;
	
	ct->fd = kqueue();
	if (ct->fd == -1) 
	{
		perror("initializing poll queue");
		return 0;
	}
	
	ct->events = calloc(max_events, sizeof(struct kevent));
	if(ct->events == NULL)
	{
		return 0;
	}

	return 1;
	
}

void evio_kqueue_free(struct ev_ct *ct)
{
	close(ct->fd);
	free(ct->events);
}

int evio_kqueue_ctl(struct ev_ct *ct, int func, int fd, int events, void *data) 
{	
	struct kevent ev;
	const struct timespec nullts = {0, 0};

	EV_SET(&ev, fd, events, func, 0, 0, data);

	if(kevent(ct->fd, &ev, 1, NULL, 0, &nullts) < 0) 
	{
		return 0;
	}
	
	return 1;
}

int evio_kqueue_mod(struct ev_ct *ct, int fd, int events, void *data) 
{
}

int evio_kqueue_add(struct ev_ct *ct, int fd, int events, void *data) 
{	
	return evio_kqueue_ctl(ct, EV_ADD, fd, events, data);
}

int evio_kqueue_del(struct ev_ct *ct, int fd) 
{
	
	return evio_kqueue_ctl(ct, EV_DELETE, fd, EVFILT_READ, NULL);
}

int evio_kqueue_wait(struct ev_ct *ct, int timeout, int fd, evio_call_accept _accept, evio_call_client _client) 
{
	int n, i;
	struct timespec ts;
	struct timespec *pts;

	if (timeout < 0) 
	{
		pts = NULL;
	}
	else
	{
		ts.tv_sec = timeout / 1000;
		ts.tv_nsec = (timeout % 1000) * 1000000;
		pts = &ts;
	}

	for( ; ; )
	{
		n = kevent(ct->fd, NULL, 0, ct->events, ct->max_events, pts);
		if(n == -1)
		{
			perror("kevent");
		}

		for (i = 0;i < n;i++)
		{
			if (ct->events[i].ident == fd)
            {
				_accept(ct->events[i].ident);
            }
            else
            {
				_client(ct->events[i].udata);
            }
		}
	}

}

struct ev_method em = {
	.name   = "kqueue",
	.free   = evio_kqueue_free,
	.init   = evio_kqueue_init,
	.wait   = evio_kqueue_wait,
	.add    = evio_kqueue_add,
	.mod    = evio_kqueue_mod,
	.del    = evio_kqueue_del,
};

