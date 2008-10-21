#define _GNU_SOURCE
#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define SPLICE_SIZE	(64*1024)

#define min(x,y) ({ \
        typeof(x) _x = (x);     \
        typeof(y) _y = (y);     \
        (void) (&_x == &_y);            \
        _x < _y ? _x : _y; })

#define max(x,y) ({ \
        typeof(x) _x = (x);     \
        typeof(y) _y = (y);     \
        (void) (&_x == &_y);            \
        _x > _y ? _x : _y; })


static int usage(char *name)
{
	fprintf(stderr, "%s: target port\n", name);
	return 1;
}

static int client_splice_loop(int out_fd, int fd, int *pfd)
{
	struct timeval start;
	unsigned long long size;
	unsigned long msecs;
	struct stat sb;

	loff_t off;

	if (fstat(fd, &sb) < 0)
		return error("fstat");

	gettimeofday(&start, NULL);

	size = sb.st_size;
	off = 0;

	do {
		int ret = splice(fd, &off, pfd[1], NULL, min(size, (unsigned long long) SPLICE_SIZE), 0);//SPLICE_F_NONBLOCK

		if (ret <= 0)
			return error("splice-in");

		size -= ret;
		while (ret > 0) {
			int flags = size ? SPLICE_F_MORE : 0;
			int written = splice(pfd[0], NULL, out_fd, NULL, ret, flags);

			if (written <= 0)
				return error("splice-out");

			ret -= written;
		}
	} while (size);


	return 0;
}

static int client_splice(int out_fd, int file_fd)
{
	int pfd[2], ret;

	if (pipe(pfd) < 0)
		return error("pipe");
	
	ret = client_splice_loop(out_fd, file_fd, pfd);
	close(pfd[0]);
	close(pfd[1]);
	return ret;
}

int main(int argc, char *argv[])
{
	struct sockaddr_in addr;
	unsigned short port;
	int fd, ret, fp;

/*
	if (argc < 3)
		return usage(argv[0]);

	if (check_input_pipe())
		return usage(argv[0]);
*/
	port = atoi(argv[2]);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if (inet_aton(argv[1], &addr.sin_addr) != 1) 
	{
		struct hostent *hent = gethostbyname(argv[1]);

		if (!hent)
		{
			return error("gethostbyname");
		}
		memcpy(&addr.sin_addr, hent->h_addr, 4);
	}

	printf("Connecting to %s/%d\n", argv[1], port);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return error("socket");

	if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		return error("connect");
	
	fp = open("sock_epoll.c", O_RDONLY);

	client_splice(fd, fp);
	/*
	do {
		ret = ssplice(STDIN_FILENO, NULL, fd, NULL, SPLICE_SIZE, SPLICE_F_NONBLOCK);
		if (ret < 0) {
			if (errno == EAGAIN) 
			{
				usleep(100);
				continue;
			}
			return error("splice");
		} else if (!ret)
			break;
	} while (1);
	*/
	close(fd);
	close(fp);
	return 0;
}
