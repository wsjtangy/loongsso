#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#define  MAX_FD        15000
#define  MAX_PROCESS   3
#define BUFFER         "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nConnection: close\r\nContent-Type: text/html\r\n\r\nHello"


struct ev_ct
{
	int                 efd;
	unsigned int        max_events;
	struct epoll_event  *events;
};


int   listenfd; 
pid_t pid[MAX_PROCESS];


int evio_epoll_init(struct ev_ct *ct, int max_events)
{
	ct->max_events  = max_events;
	
	ct->efd = epoll_create(max_events);
	if (ct->efd == -1) 
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

int evio_epoll_ctl(struct ev_ct *ct, int func, int fd, int events) 
{
	struct epoll_event ev;
	
	ev.events   = events;
	ev.data.fd  = fd;

	return epoll_ctl(ct->efd, func, fd, &ev);
}

int evio_epoll_add(struct ev_ct *ct, int fd, int events) 
{
	return evio_epoll_ctl(ct, EPOLL_CTL_ADD, fd, events);
}

int evio_epoll_mod(struct ev_ct *ct, int fd, int events) 
{
	return evio_epoll_ctl(ct, EPOLL_CTL_MOD, fd, events);
}

int evio_epoll_del(struct ev_ct *ct, int fd) 
{
	return evio_epoll_ctl(ct, EPOLL_CTL_DEL, fd, 0);
}


static void sock_set_reuseaddr(int fd)
{
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0)
	{
		//log_debug(__FILE__, __LINE__, "%s. FD %d\n", strerror(errno), fd);
	}
}

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

static void sock_listen_open(unsigned short port)
{
	struct sockaddr_in srv;
	int sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd == -1) 
	{
		printf("sock_listen_open %s\n", strerror(errno));
		exit(0);
	}
	
	sock_set_reuseaddr(sd);

	srv.sin_family        = AF_INET;
	srv.sin_port          = htons(port);
	srv.sin_addr.s_addr   = htonl(INADDR_ANY);
	
	if(-1 == bind(sd, (struct sockaddr *) &srv, sizeof(srv))) 
	{
		printf("sock_listen_open %s\n", strerror(errno));
		exit(0);
	}

	listen(sd, MAX_FD);

	listenfd = sd;
}

void http_request_write(struct ev_ct *ct, int fd)
{
	int len;
	char buf[128] = {0};

//	len = sprintf( buf, "%u: ", (unsigned int) getpid() );
//	write( fd, buf, len );
	
	write(fd, BUFFER, strlen(BUFFER));
	
	evio_epoll_del(ct, fd);
	close(fd);
}

void http_request_read(struct ev_ct *ct, int fd)
{
	ssize_t bytes;
	fd_set readfds;
	struct timeval tv;
	char buffer[1024];
	
	bytes = 0;
	memset(&buffer, 0, sizeof(buffer));

	tv.tv_sec  = 10;
	tv.tv_usec = 0;
	
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	
	select(fd+1, &readfds, NULL, NULL, &tv);
	
	if (!FD_ISSET(fd, &readfds)) 
	{
		// timeout
		return ; 
	}

	bytes = recv(fd, buffer, sizeof(buffer), 0);
	if(bytes <= 0)
	{
		evio_epoll_del(ct, fd);
		close(fd);
		return ;
	}
	
	evio_epoll_mod(ct, fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
}

void *serv_epoll(void *arg)
{
	int n, i;
	struct epoll_event *cevents;
	struct ev_ct *ct = (struct ev_ct *)arg;

	for( ; ; )
    {
		n = epoll_wait(ct->efd, ct->events, ct->max_events, -1);
		if (n == -1) continue;

	//	printf("pid = %d\r\n", getpid());
		
        for (i = 0, cevents = ct->events; i < n; i++, cevents++)
        {
			if(cevents->events & (EPOLLHUP | EPOLLERR))
			{
				evio_epoll_del(ct, cevents->data.fd);
				close(cevents->data.fd);
			}
			if(cevents->events & EPOLLIN)
			{
				http_request_read(ct, cevents->data.fd);
			}
			else if(cevents->events & EPOLLOUT)
			{
				http_request_write(ct, cevents->data.fd);
			}
        }
    }

}


static void child_main()
{
	int connfd;
	struct ev_ct ct;
	
	pthread_t tid;
    pthread_attr_t attr;
	struct sockaddr_in addr;
	int socklen = sizeof(struct sockaddr);
	
	evio_epoll_init(&ct, MAX_FD);
	
	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, serv_epoll, (void *)&ct) != 0)
    {
          printf("pthread_create failed\n");
          return ;
    }

	for(; ;)
	{
		connfd = accept(listenfd, (struct sockaddr*)&addr, (socklen_t *)&socklen);

		if (connfd < 0) 
		{
			printf("Accept returned an error (%s) ... retrying.", strerror(errno));
			continue;
		}
	
		sock_set_options(connfd);
		evio_epoll_add(&ct, connfd, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP);
	}
}



void kill_children()
{
	int i;
	
	for(i = 0; i<MAX_PROCESS; i++) 
	{
		kill(pid[i], SIGTERM);
	}
}


static void server_down(int signum)
{
	kill_children();
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
	daemon(1, 1);

	int i;
	struct sigaction sa;
	
	//设定接受到指定信号后的动作为忽略
	sa.sa_handler = SIG_IGN;
	sa.sa_flags   = 0;

	//初始化信号集为空 屏蔽SIGPIPE信号
	if (sigemptyset(&sa.sa_mask) == -1 || sigaction(SIGPIPE, &sa, 0) == -1) 
	{ 
		perror("failed to ignore SIGPIPE; sigaction");
		exit(EXIT_FAILURE);
	}

	sock_listen_open(8000);
	
	for(i=0; i<MAX_PROCESS; i++)
	{
		pid[i] = fork();
		if (pid[i] == 0)
		{
			signal( SIGCHLD, SIG_DFL );
			signal( SIGTERM, SIG_DFL );
			signal( SIGHUP, SIG_DFL );

			child_main();
		}
	}

	signal(SIGTERM, server_down);
	signal(SIGINT,  server_down);
	signal(SIGQUIT, server_down);
	signal(SIGSEGV, server_down);
	signal(SIGALRM, server_down);

	for(; ;)
	{
		sleep(900);
	}
	return 1;
}

