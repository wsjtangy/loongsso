#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <libaio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>


static io_context_t *aio_queue;

int file_aio_init(int max) 
{
	int events = 16;
	if(max > 0) 
		events = max;
	
	aio_queue = calloc(events, sizeof(io_context_t));
	if(!aio_queue) 
	{
		printf("file_aio_init %s\n", strerror(errno));
		return -1;
	}
	
	if(0 != io_setup(events, aio_queue)) 
	{
		printf("file_aio_init %s\n", strerror(errno));
		return -1;
	}
	
	printf("file_aio_init success! %d events\n", events);
	return 0;
}

static void read_done(io_context_t ctx, struct iocb *iocb, long res, long res2)
{
	printf("%s", iocb->u.c.buf);
	close(iocb->aio_fildes);

	free(iocb->u.c.buf);
	free(iocb);
	return;
}

int file_aio_read(int fd, void *start, int size, long long offset)
{
	struct iocb *cb = malloc(sizeof(struct iocb)); /* remaind to free */
	if(!cb) 
	{
		printf("file_aio_write %s\n", strerror(errno));
		return -1;
	}

	io_prep_pread(cb, fd, start, size, offset);
	io_set_callback(cb, read_done);

	return io_submit(*aio_queue, 1, &cb);
}




void file_aio_loop(void) 
{
	struct io_event event[16];
	struct timespec io_ts;
	int res;
	
	io_ts.tv_sec = 0;
	io_ts.tv_nsec = 0;

	res = io_getevents(*aio_queue, 1, 16, event, &io_ts);
	printf("res = %d\r\n", res);
	if(res > 0) 
	{
		int i;
		for(i = 0; i < res; i++) 
		{
			io_callback_t callback = (io_callback_t)event[i].data;
			
			struct iocb *iocb = event[i].obj;
			callback(*aio_queue, iocb, event[i].res, event[i].res2);
		}	
	}
	else if(res < 0) 
	{
		printf("file_aio_loop %s\n", strerror(errno));
	}
}

int main(int argc, char *argv[])
{
	int i;
	char filename[2][20] = {"./lua_test.c", "./httpd.c"};
	
	file_aio_init(16);

	for(i=0; i<2; i++)
	{
		struct stat sbuf;

		int fd = open(filename[i], O_RDONLY);
		if (fd < 0) perror("open");

		fstat(fd, &sbuf);

		unsigned char *buf = calloc(sbuf.st_size + 1, sizeof(unsigned char));
		file_aio_read(fd, buf, sbuf.st_size, 0);
	}

	file_aio_loop();

	/*
	int fd, ret;
	struct stat sbuf;
	unsigned char *buf;
	
	file_aio_init(16);

	fd = open("lua_test.c", O_RDONLY );
	if (fd < 0) perror("open");
	
	printf("fd = %d\r\n", fd);
	fstat(fd, &sbuf);

	buf = calloc(sbuf.st_size + 1, sizeof(unsigned char));
	
	file_aio_read(fd, buf, sbuf.st_size, 0);
	
	file_aio_loop();
	close(fd);
	*/
	return 0;
}
