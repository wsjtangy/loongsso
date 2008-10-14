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


const char *mimetype(const char *filename)
{
	static const char *(assocNames[][2]) =
	{
		{ "html", "text/html" },
		{ "htm", "text/html" },
		{ "txt", "text/plain" },
		{ "css", "text/css" },
		{ "png", "image/png" },
		{ "gif", "image/gif" },
		{ "jpg", "image/jpeg" },
		{ "jpeg", "image/jpeg" },
		{ "bmp", "image/bmp" },
		{ "mp3", "audio/mpeg" },
		{ "mp2", "audio/mpeg" },
		{ "mpga", "audio/mpeg" },
		{ "wav", "audio/x-wav" },
		{ "mid", "audio/midi" },
		{ "midi", "audio/midi" },
		{ "ra", "audio/x-pn-realaudio" },
		{ "ram", "audio/x-pn-realaudio" },
		{ "mpeg", "video/mpeg" },
		{ "mpg", "video/mpeg" },
		{ "mpe", "video/mpeg" },
		{ "avi", "video/x-msvideo" },
		{ "xml", "application/xml" },
		{ "xsl", "application/xml" },
		{ "xslt", "application/xslt-xml" },
		{ "xhtml", "application/xhtml+xml" },
		{ "xht", "application/xhtml+xml" },
		{ "dtd", "application/xml-dtd" },
		{ "js", "application/x-javascript" },
		{ "tar", "application/x-tar" },
		{ "zip", "application/x-zip" },
		{ "", "application/octet-stream" } // must be last entry!
	};

	const char *((*anp)[2]);
	char *suffix;

	suffix = strrchr(filename, '.');
	if (suffix != NULL) 
	{
		suffix++;
		for (anp=assocNames; strlen((*anp)[0])>0; anp++)
			if (!strcmp((*anp)[0],suffix)) break;
	}

	return (*anp)[1];
}

void parse_query(char *request, char *uri, int url_len, char *query, int query_len)
{
	int i, pos, flags, len, num;
	
	num   = 0;
	pos   = 0;
	flags = 0;
	len   = strlen(request);
	for(i=0; i<len; i++)
	{
		if(request[i] == '?')
		{
			flags = 1;
		}
		else if(flags && pos < query_len)
		{
			query[num++] = request[i];
		}
		else if(!flags && pos < url_len)
		{
			uri[pos++] = request[i];
		}
		else 
		{
			break;
		}
	}

	request[pos] = '\0';
}


int request_parse(char *req_ptr, size_t req_len, int fd) 
{
	int len, pos;
	struct conn_t *conn;
	char *line_start, *line_end, *ptr, *req_end, *uri;
	
	ptr     = req_ptr;
	req_end = req_ptr + req_len;
	conn    = &server.conn[fd];

	memset(&conn->req.uri, 0, sizeof(conn->req.uri));
	memset(&conn->req.query_ptr, 0, sizeof(conn->req.query_ptr));
	memset(&conn->req.filepath, 0, sizeof(conn->req.filepath));

	for (line_end = ptr;ptr < req_end;ptr++) 
	{
		if (ptr >= line_end) 
		{
			//At the end of a line, find if we have another
			line_end = ptr;
			
			//Find start of next line
			while (ptr < req_end && (*ptr == '\n' || *ptr == '\r')) 
			{
				//If null line found return 1
				if (*ptr == '\n' && (*(ptr - 1) == '\n' || *(ptr - 2) == '\n'))
					return 1;
				ptr++;
			}
			
			line_start = ptr;
			
			//Find end of line
			while (ptr < req_end && *ptr != '\n' && *ptr != '\r') ptr++;
			
			line_end = ptr - 1;
			ptr      = line_start;
		}
		
		switch (*ptr) 
		{
			case ':':
				ptr += 2;
				switch (*line_start) 
				{
				/*	case 'H':
						if ((line_start + 3) < req_end && !strncmp(line_start+1, "ost", 3)) 
						{
							len = line_end - ptr;
							conn->req.host = malloc(len + 1);
							memcpy(conn->req.host, ptr, len);
							conn->req.host[len] = '\0';
						}
						break;*/
					case 'C':
						if ((line_start + 9) < req_end && !strncmp(line_start+1, "onnection", 9)) 
						{
							if ((ptr + 10) < req_end && !strncmp(ptr, "keep-alive", 10))
								conn->req.keep_alive = 1;
							else if ((ptr + 5) < req_end && !strncmp(ptr, "close", 5))
								conn->req.keep_alive = 0;
						}
				}
				ptr = line_end;
				break;
			case ' ':
				switch (*line_start) 
				{
					case 'G':
						if ((line_start + 3) < req_end && !strncmp(line_start+1, "ET", 2))
							conn->req.http_method = HTTP_METHOD_GET;
						break;
					case 'P':
						if ((line_start + 4) < req_end && !strncmp(line_start+1, "OST", 3))
							conn->req.http_method = HTTP_METHOD_POST;
						break;
					case 'H':
						if ((line_start + 4) < req_end && !strncmp(line_start+1, "EAD", 3))
							conn->req.http_method = HTTP_METHOD_POST;
				}
				
				line_start = ptr;
				do ptr++; while (ptr < line_end && *ptr != ' ');
				
				if (*ptr == ' ' && (ptr + 8) == line_end && !strncmp(ptr+1, "HTTP/1.", 7)) 
				{
					if (*(ptr+8) == '0')
						conn->req.http_version = HTTP_VERSION_1_0;
					else if (*(ptr+8) == '1')
						conn->req.http_version = HTTP_VERSION_1_1;
					
					len = ptr - line_start;
					pos = len <= sizeof(conn->req.filepath) ? len : sizeof(conn->req.filepath); 
					memcpy(conn->req.filepath, line_start+1, pos-1);
					conn->req.filepath[pos-1] = '\0';
					
					uri  = strrchr(conn->req.filepath, '/');
					if(uri)
					{
						uri++;
						parse_query(uri, conn->req.uri, sizeof(conn->req.uri), conn->req.query_ptr, sizeof(conn->req.query_ptr));
					}
				}

				ptr = line_end;
				break;
		}
	}
	
	return 1;
}


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
	struct conn_t *conn;
	
	bytes = 0;
	conn  = &server.conn[fd];
	memset(&buffer, 0, sizeof(buffer));

	tv.tv_sec  = 10;
	tv.tv_usec = 0;
	
	FD_ZERO(&readfds);
	FD_SET(conn->fd, &readfds);
	
	select(conn->fd+1, &readfds, NULL, NULL, &tv);
	
	if (!FD_ISSET(conn->fd, &readfds)) 
	{
		// timeout
		return ; 
	}

	bytes = recv(conn->fd, buffer, sizeof(buffer), 0);
	if(bytes <= 0)
	{
		sock_close(conn->fd);
		return ;
	}

//	printf("%s", buffer);
	
	request_parse(buffer, bytes, conn->fd);

	
	printf("uri = %s\r\n", conn->req.uri);
	printf("query = %s\r\n", conn->req.query_ptr);
	printf("filepath = %s\r\n", conn->req.filepath);
	
	sock_epoll_mod(fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
}


int main(int argc, char *argv[])
{
//	daemon(1, 1);

	sock_init();
	sock_epoll_wait(-1);
	return 0;
}
