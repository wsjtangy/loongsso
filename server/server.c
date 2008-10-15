#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/uio.h>
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
		{ "c",       "text/plain"                         },
		{ "js",      "text/javascript"                    },
		{ "gz",      "application/x-gzip"                 },
		{ "ps",      "application/postscript"             },
		{ "pdf",     "application/pdf"                    },
		{ "dvi",     "application/x-dvi"                  },
		{ "tgz",     "application/x-tgz"                  },
		{ "tar",     "application/x-tar"                  },
		{ "zip",     "application/zip"                    },
		{ "mp3",     "audio/mpeg"                         },
		{ "m3u",     "audio/x-mpegurl"                    },
		{ "wma",     "audio/x-ms-wma"                     },
		{ "wax",     "audio/x-ms-wax"                     },
		{ "ogg",     "application/ogg"                    },
		{ "wav",     "audio/x-wav"                        },
		{ "gif",     "image/gif"                          },
		{ "jar",     "application/x-java-archive"         },
		{ "jpg",     "image/jpeg"                         },
		{ "jpeg",    "image/jpeg"                         },
		{ "png",     "image/png"                          },
		{ "xbm",     "image/x-xbitmap"                    },
		{ "xpm",     "image/x-xbitmap"                    },
		{ "xwd",     "image/x-xwindowdump"                },
		{ "css",     "text/css"                           },
		{ "asc",     "text/plain"                         },
		{ "cpp",     "text/plain"                         },
		{ "htm",     "text/html"                          },
		{ "html",    "text/html"                          },
		{ "log",     "text/plain"                         },
		{ "txt",     "text/plain"                         },
		{ "conf",    "text/plain"                         },
		{ "dtd",     "text/xml"                           },
		{ "xml",     "text/xml"                           },
		{ "mov",     "video/quicktime"                    },
		{ "mpg",     "video/mpeg"                         },
		{ "mpeg",    "video/mpeg"                         },
		{ "qt",      "video/quicktime"                    },
		{ "avi",     "video/x-msvideo"                    },
		{ "asf",     "video/x-ms-asf"                     },
		{ "asx",     "video/x-ms-asf"                     },
		{ "wmv",     "video/x-ms-wmv"                     },
		{ "bz2",     "application/x-bzip"                 },
		{ "tbz",     "application/x-bzip-compressed-tar"  },
		{ "sig",     "application/pgp-signature"          },
		{ "spl",     "application/futuresplash"           },
		{ "swf",     "application/x-shockwave-flash"      },
		{ "pac",     "application/x-ns-proxy-autoconfig"  },
		{ "class",   "application/octet-stream"           },
		{ "tar.gz",  "application/x-tgz"                  },
		{ "torrent", "application/x-bittorrent"           },
		{ "tar.bz2", "application/x-bzip-compressed-tar"  },
		{ "",        "application/octet-stream"           }
	};

	const char *((*anp)[2]);
	char *suffix;

	suffix = strrchr(filename, '.');
	if (suffix != NULL) 
	{
		suffix++;
		for (anp=assocNames; strlen((*anp)[0])>0; anp++)
			if (!strcmp((*anp)[0], suffix)) break;
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

	memset(&conn->req.uri,       0, sizeof(conn->req.uri));
	memset(&conn->req.filepath,  0, sizeof(conn->req.filepath));
	memset(&conn->req.query_ptr, 0, sizeof(conn->req.query_ptr));

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

int file_load_memory(char *filename)
{
	int fd;
	struct stat sbuf;
	unsigned char *buf;

	fd = open(filename, O_RDONLY);
	if(fd == -1) return 0;

	fstat(fd, &sbuf);

	buf = calloc(sbuf.st_size + 1, sizeof(unsigned char));
	if(!buf)
	{
		close(fd);
		return 0;
	}

	read(fd, buf, sbuf.st_size);
	close(fd);

	hashmap_add(memdisk, filename, buf, sbuf.st_size, sbuf.st_mtime);

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

void http_reply_header(const struct record *recs, struct conn_t *conn)
{
	int  header_len;
	char header[400];
	char timebuf[100];
	char lastime[100];
	struct iovec vectors[2];

	memset(&header, 0, sizeof(header));
	memset(&timebuf, 0, sizeof(timebuf));
	memset(&lastime, 0, sizeof(lastime));
	
	strftime(timebuf, sizeof(timebuf), RFC1123, localtime(&conn->now));
	strftime(lastime, sizeof(lastime), RFC1123, localtime(&recs->file_time));

	header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nServer: qiye/RC1.2\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nLast-Modified: %s\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\n\r\n", timebuf, mimetype(recs->path), recs->length, lastime);
	
//	printf("header = %s\r\nrecs->length = %u\r\n", header, recs->length);
	vectors[0].iov_base = header;
	vectors[0].iov_len  = header_len;
	vectors[1].iov_base = recs->content;
	vectors[1].iov_len  = recs->length;

	writev(conn->fd, vectors, 2);

}

void http_request_write(int fd)
{
	int rc;
	ssize_t bytes;
	char filename[200];
	struct conn_t *conn;
	const struct record *recs;
	
	bytes = 0;
	conn  = &server.conn[fd];

	memset(&filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename), "%s%s", server.root, conn->req.filepath);
	
	recs  = hashmap_get(memdisk, filename);
	if(recs)
	{
		http_reply_header(recs, conn);
		return ;
	}
	rc = file_load_memory(filename);
	if(rc)
	{
		recs = hashmap_get(memdisk, filename);
		if(recs)
		{
			http_reply_header(recs, conn);
			return ;
		}
	}

	bytes = send(conn->fd, BUFFER, strlen(BUFFER), 0);
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

	
//	printf("uri = %s\r\n", conn->req.uri);
//	printf("query = %s\r\n", conn->req.query_ptr);
//	printf("filepath = %s\r\n", conn->req.filepath);
	
	sock_epoll_mod(fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
}


static void server_down(int signum)
{
	sock_epoll_free();
	hashmap_destroy(memdisk);
	exit(1);
}

int main(int argc, char *argv[])
{
//	daemon(1, 1);
	
//	printf("ext = %s\r\n", mimetype("/www/core/thread-1271438-1-1.html"));

	sock_init();

	memdisk = hashmap_new(100);

	signal(SIGTERM, server_down);
	signal(SIGINT,  server_down);
	signal(SIGQUIT, server_down);
	signal(SIGSEGV, server_down);
	signal(SIGALRM, server_down);
	signal(SIGPIPE, server_down);

	sock_epoll_wait(-1);
	return 0;
}

/*

HTTP/1.1 200 OK
Server: qiye/RC1.2
Date: Wed, 15 Oct 2008 09:26:37 GMT
Content-Type: application/x-javascript
Content-Length: 7750
Last-Modified: Mon, 15 Sep 2008 08:42:04 GMT
Connection: keep-alive
Accept-Ranges: bytes

*/

