#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "server.h"
#include "sock_io.h"

#define BUFFER "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nConnection: close\r\nContent-Type: text/html\r\n\r\nHello"

void http_request_accept(int fd)
{
	struct sockaddr_in addr;
	int socklen = sizeof(struct sockaddr);

	int client_fd = accept(fd, (struct sockaddr*)&addr, (socklen_t *)&socklen);
	
	if(client_fd == -1)
	{
		printf("http_request_accept %s! Exit.\n", strerror(errno));
		return;
	}
	
	fd_open(client_fd);

	sock_epoll_add(client_fd, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP);
}

void http_request_write(int fd)
{
	ssize_t bytes;
	
	bytes = 0;
	bytes = send(fd, BUFFER, strlen(BUFFER), 0);
	if(bytes <= 0)
	{
		sock_close(fd);
		return ;
	}
}

void http_request_read(int fd)
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
		sock_close(fd);
		return ;
	}

//	printf("%s", buffer);
	
	sock_epoll_mod(fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
}


int main(int argc, char *argv[])
{
	daemon(1, 1);

	sock_init();
	sock_epoll_wait(-1);
	return 0;
}
