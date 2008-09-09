#ifndef __EVIO_KQUEUE__
#define __EVIO_KQUEUE__

#include <sys/event.h>

#define EVIO_IN	    EVFILT_READ
#define EVIO_OUT    EVFILT_WRITE 

typedef void (*evio_call_client)(void *data);
typedef void (*evio_call_accept)(int sockfd);

struct ev_ct
{
	int                      fd;	
	unsigned int             max_events;
	unsigned int             change_count;
	struct kevent            *events;
};

struct ev_method 
{
	const char *name;
	void (*free)(struct ev_ct *ct);
	int  (*init)(struct ev_ct *ct, int max_events);
	int  (*wait)(struct ev_ct *ct, int timeout, int fd, evio_call_accept _accept, evio_call_client _client);
	int  (*add)(struct ev_ct *ct, int fd, int events, void *data);
	int  (*mod)(struct ev_ct *ct, int fd, int events, void *data);
	int  (*del)(struct ev_ct *ct, int fd);
};


extern struct ev_method em;

#endif

