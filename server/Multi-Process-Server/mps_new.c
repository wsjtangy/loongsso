#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include "mps_new.h"
#include "hashmap.h"

#define  MAX_FD        15000
#define  MAX_PROCESS   3
#define BUFFER         "HTTP/1.1 200 OK\r\nContent-Length: 5\r\nConnection: close\r\nContent-Type: text/html\r\n\r\nHello"

hashmap             *mc;
int                 listenfd; 
struct mps_process  processes[MAX_PROCESS];

volatile sig_atomic_t signal_exitc = 1;

void sigterm_handler (int sig)
{
	signal_exitc = 0;
	_exit(EXIT_SUCCESS);
}

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


int request_parse(struct server_t *server, char *req_ptr, size_t req_len, int fd) 
{
	int len, pos;
	struct conn_t *conn;
	char *line_start, *line_end, *ptr, *req_end, *uri;
	
	ptr     = req_ptr;
	req_end = req_ptr + req_len;
	conn    = &server->conn[fd];

	memset(&conn->req.uri,       0, sizeof(conn->req.uri));
	memset(&conn->req.filepath,  0, sizeof(conn->req.filepath));
	memset(&conn->req.query_ptr, 0, sizeof(conn->req.query_ptr));
	memset(&conn->req.if_modified_since, 0, sizeof(conn->req.if_modified_since));

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
					case 'I':
						if ((line_start + 3) < req_end && !strncmp(line_start+1, "f-Modified-Since", 16)) 
						{
							len = line_end - ptr;
							pos = (len+1) <= sizeof(conn->req.if_modified_since) ? (len+1) : sizeof(conn->req.if_modified_since); 
							memcpy(conn->req.if_modified_since, ptr, pos);
						}
						break;
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

int sock_set_noblocking(int fd) 
{
	int flag;
	int dummy = 0;
	
	if(-1 == (flag = fcntl(fd, F_GETFL, dummy))) 
	{
		printf("%s. fd %d\n", strerror(errno), fd);
		return -1;
	}

	if(-1 == (flag = fcntl(fd, F_SETFL, dummy | O_NONBLOCK | O_NDELAY))) 
	{
		printf("%s. fd %d\n", strerror(errno), fd);
		return -1;
	}

	return 0;
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

void fd_open(struct server_t *server, int fd)
{
	struct conn_t *c = &server->conn[fd];

	c->fd  = fd;
	c->now = time((time_t*)0);
	if(fd > server->maxfd)
	{
		server->maxfd = fd;
	}
	sock_set_options(fd);
}

static void fd_close(struct server_t *server, int fd)
{
	struct conn_t *c = &server->conn[fd];

	c->fd = 0;
	if(fd >= server->maxfd) 
	{
		server->maxfd--;
	}
	close(fd);
}

void sock_close(struct server_t *server, int fd) 
{	
	struct conn_t *c = &server->conn[fd];
	if(!c->fd)
		return;
	
	close(c->req.fd);
	c->req.fd     = -1;
	c->req.size   = 0;
	c->req.length = 0;

	evio_epoll_del(&server->ct, fd);
	fd_close(server, fd);
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


void http_request_error(struct server_t *server, struct conn_t *conn, char *cmd)
{
	ssize_t bytes;
	char body[400];
	char header[400];
	char timebuf[100];
	struct iovec vectors[2];
	int  header_len, body_len;

	bytes = 0;
	memset(&body, 0, sizeof(body));
	memset(&header, 0, sizeof(header));
	memset(&timebuf, 0, sizeof(timebuf));
	
	strftime(timebuf, sizeof(timebuf), RFC_TIME, localtime(&conn->now));

	if(strcmp(cmd, "404 Not Found") == 0)
	{
		body_len   = snprintf(body, sizeof(body), "<html>\r\n<head>\r\n<title>404 Not Found</title>\r\n</head>\r\n<body>\r\n<h1>Not Found</h1><p>The requested URL %s was not found on this server.</p><hr><address>Memhttpd/Beta1.0</address>\r\n</body>\r\n</html>", conn->req.filepath);
		header_len = snprintf(header, sizeof(header), RFC_404, cmd, timebuf, body_len);

		vectors[0].iov_base = header;
		vectors[0].iov_len  = header_len;
		vectors[1].iov_base = body;
		vectors[1].iov_len  = body_len;

		bytes = writev(conn->fd, vectors, 2);
	}
	else
	{
		header_len = snprintf(header, sizeof(header), RFC_304, cmd, timebuf);
		bytes = send(conn->fd, header, header_len, 0);
	}
	sock_close(server, conn->fd);
}

void http_reply_body(struct server_t *server, struct conn_t *conn)
{
	off_t   pos;
	ssize_t bytes;
	
	pos   = conn->req.size;
	bytes = 0;
	
	bytes = sendfile(conn->fd, conn->req.fd, &pos , (conn->req.length - conn->req.size));
	if(bytes < 0)
	{
		switch (errno) 
		{
			case EAGAIN:
			case EINTR:
				bytes = 0;
				sock_close(server, conn->fd);
				return ;
			case EPIPE:
			case ECONNRESET:
				sock_close(server, conn->fd);
				return ;
			default:
				sock_close(server, conn->fd);
				return ;
		}
	}
	else if(conn->req.length > (bytes + conn->req.size))
	{
		conn->now       = time((time_t*)0);
		conn->req.size += bytes;
		evio_epoll_mod(&server->ct, conn->fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
		return ;
	}

	sock_close(server, conn->fd);
}

void http_reply_header(struct server_t *server, struct conn_t *conn, time_t file_time)
{
	ssize_t bytes;
	int  header_len;
	char header[400];
	char timebuf[100];
	char lastime[100];
	
	bytes = 0;
	memset(&header, 0, sizeof(header));
	memset(&timebuf, 0, sizeof(timebuf));
	memset(&lastime, 0, sizeof(lastime));
	
	strftime(timebuf, sizeof(timebuf), RFC_TIME, localtime(&conn->now));
	strftime(lastime, sizeof(lastime), RFC_TIME, localtime(&file_time));
	
	if(strcmp(lastime, conn->req.if_modified_since) == 0)
	{
		http_request_error(server, conn, "304 Not Modified");
		return ;
	}

	header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nServer: Memhttpd/Beta1.0\r\nDate: %s\r\nContent-Type: %s\r\nContent-Length: %u\r\nLast-Modified: %s\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\n\r\n", timebuf, mimetype(conn->req.uri), conn->req.length, lastime);
	
	bytes = write(conn->fd, header, header_len);
	if(bytes != header_len)
	{
		sock_close(server, conn->fd);
		return ;
	}

	conn->now  = time((time_t*)0);
	evio_epoll_mod(&server->ct, conn->fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
}

void http_request_write(struct server_t *server, int fd)
{
	int rc;
	time_t ftime;
	struct stat sbuf;
	char filename[200];
	struct conn_t *conn;
	const struct record *recs;
	
	conn  = &server->conn[fd];
	
	if(conn->req.http_method != HTTP_METHOD_GET)
	{
		http_request_error(server, conn, "505 HTTP Version not supported");
		return ;
	}
	if((conn->req.fd > 0) && (conn->req.size >= 0) && (conn->req.length > 0))
	{
		http_reply_body(server, conn);
		return ;
	}

	memset(&filename, 0, sizeof(filename));
	snprintf(filename, sizeof(filename), "%s%s", server->root, conn->req.filepath);
	conn->req.fd = open(filename, O_RDONLY);
	if(conn->req.fd == -1)
	{
		http_request_error(server, conn, "404 Not Found");
		return ;
	}
	
//	printf("filename = %s\r\n", filename);

	recs = hashmap_get(mc, conn->req.filepath);
	if(recs)
	{
//		printf("size = %u\tfile_time = %u\r\n", recs->length, recs->file_time);
		conn->req.size   = 0;
		conn->req.length = recs->length;
		ftime            = recs->file_time;
	}
	else
	{
		fstat(conn->req.fd, &sbuf);
	
		conn->req.size   = 0;
		conn->req.length = sbuf.st_size;
		ftime            = sbuf.st_mtime;

		hashmap_add(mc, conn->req.filepath, sbuf.st_size, sbuf.st_mtime);
	}

//	printf("http_reply_header\r\n");
	http_reply_header(server, conn, ftime);
}

void http_request_read(struct server_t *server, int fd)
{
	ssize_t bytes;
	fd_set readfds;
	struct timeval tv;
	char buffer[1024];
	struct conn_t *conn;
	
	bytes = 0;
	conn  = &server->conn[fd];
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
		sock_close(server, conn->fd);
		return ;
	}
	
	request_parse(server, buffer, bytes, conn->fd);
	
	conn->now  = time((time_t*)0);
	evio_epoll_mod(&server->ct, fd, EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP);
}

void *sock_listen_event(void *arg)
{
	int n, i;
	ssize_t bytes;
	char    buf[4];
	time_t epoll_time;
	struct epoll_event *cevents;
	struct server_t *server = (struct server_t *)arg;
	
	//printf("sock_listen_event\tpid = %lu\tproc->channel[0] = %d\r\n", getpid(), server->proc->channel[0]);
	for( ; ; )
    {
		n = epoll_wait(server->ct.efd, server->ct.events, server->ct.max_events, -1);
		if (n == -1) continue;

		// check timeout 
		epoll_time = time((time_t*)0);
		for(i = 0; i <= server->maxfd; i++)
		{
			struct conn_t *c = &server->conn[i];
			if(c->fd && (epoll_time > (c->now + SOCK_TIMEOUT)))
			{
				sock_close(server, c->fd);
			}
		}
		
        for (i = 0, cevents = server->ct.events; i < n; i++, cevents++)
        {
			if(cevents->events & (EPOLLHUP | EPOLLERR))
			{
				sock_close(server, cevents->data.fd);
			}
			else if(cevents->events & EPOLLIN && (cevents->data.fd == server->proc->channel[0]))
			{
				//维持和父进程的心跳包
				memset(&buf, 0, sizeof(buf));
				bytes = read(cevents->data.fd, buf, sizeof(buf));
				
				write(cevents->data.fd, "c", 1);
			//	printf("子进程 write\r\n");
			}
			else if(cevents->events & EPOLLIN)
			{
			//	printf("http_request_read开始\r\n");
				http_request_read(server, cevents->data.fd);
			//	printf("http_request_read结束\r\n");
			}
			else if(cevents->events & EPOLLOUT)
			{
			//	printf("http_request_write开始\r\n");
				http_request_write(server, cevents->data.fd);
			//	printf("http_request_write结束\r\n");
			}
        }
    }
}


static void child_main(struct mps_process *proc)
{
	int connfd; 
	pthread_t tid;
	struct rlimit rt;
    pthread_attr_t attr;
	struct sockaddr_in addr;
	struct sigaction sigterm;

	struct server_t server;
	int socklen = sizeof(struct sockaddr);


	rt.rlim_max = 4096;
	rt.rlim_cur = 1024;
	
/*	if (setrlimit(RLIMIT_NOFILE, &rt) == -1) 
	{
		perror("setrlimit");
		_exit(EXIT_FAILURE);
	}
*/	
	if (sigaction(SIGTERM, &sigterm, NULL) < 0)
	{
		perror("unable to setup signal handler for SIGTERM");
		_exit(EXIT_FAILURE);
	}

	server.proc  = proc;
	server.root  = "/home/lijinxing/server/www";
	server.conn  = calloc(MAX_FD, sizeof(struct conn_t));
	evio_epoll_init(&server.ct, MAX_FD);
	
	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &attr, sock_listen_event, (void *)&server) != 0)
    {
          printf("pthread_create failed\n");
          _exit(EXIT_FAILURE);
    }
	
	evio_epoll_add(&server.ct, proc->channel[0], EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP);
	
	while (signal_exitc)
	{
		connfd = accept(listenfd, (struct sockaddr*)&addr, (socklen_t *)&socklen);

		if (connfd < 0) 
		{
			printf("Accept returned an error (%s) ... retrying.", strerror(errno));
			continue;
		}

		fd_open(&server, connfd);
		evio_epoll_add(&server.ct, connfd, EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLHUP);
	}
}



void kill_children()
{
	int i;
	
	for(i = 0; i<MAX_PROCESS; i++) 
	{
		kill(processes[i].pid, SIGTERM);
	}
}


static void server_down(int signum)
{
	close(listenfd);
	kill_children();
	hashmap_destroy(mc);
	exit(EXIT_SUCCESS);
}

void mps_spawn_process(struct mps_process *proc)
{
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, proc->channel) == -1)
	{
		printf("Call socketpair error, errno is %d\n", errno);
		server_down(0);
	}

	proc->pid = fork();
	if(proc->pid == -1)
	{
		//fork error
		server_down(0);
	}
	else if (proc->pid == 0)
	{
		close(proc->channel[1]);

		sock_set_noblocking(proc->channel[0]);
		child_main(proc);
	}

	close(proc->channel[0]);
}

int main(int argc, char *argv[])
{
	daemon(1, 1);

	int i;
	ssize_t n;
	char   buf[3];
	fd_set readfds;
	struct timeval tv;
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
	signal(SIGCLD, SIG_IGN);

	mc  = hashmap_new(769);
	sock_listen_open(8000);
	
	for(i=0; i<MAX_PROCESS; i++)
	{
		mps_spawn_process(&processes[i]);
	}

	signal(SIGTERM, server_down);
	signal(SIGINT,  server_down);
	signal(SIGQUIT, server_down);
	signal(SIGSEGV, server_down);
	signal(SIGALRM, server_down);

	for(; ;)
	{
		sleep(200);

		//父子进程的心跳包,失去心跳的子进程将kill掉
		for(i=0; i<MAX_PROCESS; i++)
		{
			memset(&buf, 0, sizeof(buf));
			n = write(processes[i].channel[1], "p", 1);
			if(n < 0)
			{
				printf("write kill %lu\r\n", processes[i].pid);
				kill(processes[i].pid, SIGTERM);
				close(processes[i].channel[1]);
				mps_spawn_process(&processes[i]);
				continue;
			}

			tv.tv_sec  = 20;
			tv.tv_usec = 0;
			
			FD_ZERO(&readfds);
			FD_SET(processes[i].channel[1], &readfds);
			
			select(processes[i].channel[1]+1, &readfds, NULL, NULL, &tv);
			
			if (!FD_ISSET(processes[i].channel[1], &readfds)) 
			{
				// timeout
				printf("timeout kill %lu\r\n", processes[i].pid);
				kill(processes[i].pid, SIGTERM);
				close(processes[i].channel[1]);
				mps_spawn_process(&processes[i]);
				continue;
			}
			
			n = read(processes[i].channel[1], buf, sizeof(buf));
			if(n < 0)
			{
				printf("read kill %lu\r\n", processes[i].pid);
				kill(processes[i].pid, SIGTERM);
				close(processes[i].channel[1]);
				mps_spawn_process(&processes[i]);
			}
		}
	}
	return 1;
}

