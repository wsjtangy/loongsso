#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define  MAX_THREAD  10

typedef void PF(void *);

typedef enum 
{
	THREADPOOL_FREE,
	THREADPOOL_BUSY
} tp_status;

struct threadpool
{
	pthread_t       id;
	pthread_cond_t  cond;
	pthread_mutex_t mutex;
	unsigned int    index;
	tp_status       status;
	PF              *handle;
	void            *data;
};

struct threadpool tp[MAX_THREAD];

//让线程睡眠
void threadpool_sleep(int id)
{
	tp[id].status = THREADPOOL_FREE;
	pthread_mutex_lock (&tp[id].mutex);

	while (tp[id].status == THREADPOOL_FREE)
	{ 
	   pthread_cond_wait (&tp[id].cond, &tp[id].mutex); 
	}
	pthread_mutex_unlock (&tp[id].mutex);
}

//寻找一个睡眠的线程
int threadpool_fetch(PF *handle, void *data)
{
	int i;

	for(i=0; i<MAX_THREAD; i++)
	{
		if(i == THREADPOOL_FREE)
		{
			tp[i].status = THREADPOOL_BUSY;
			tp[i].handle = handle;
			tp[i].data   = data;

			pthread_cond_signal (&tp[i].cond);
			return 1;
		}
	}

	return 0;
}

void *threadpool_wait(void *arg)
{
	void *data;
	PF   *handle;
	int index = *(int *)arg;
	
	for(; ;)
	{
		threadpool_sleep(index);
		
		data   = tp[index].data;
		handle = tp[index].handle;
		handle(data);

		data = handle = NULL;
	}
}

int threadpool_init(unsigned int num)
{
	int i;

	for(i=0; i<MAX_THREAD; i++)
	{
		tp[i].status = THREADPOOL_FREE;
		tp[i].index  = i;
		pthread_mutex_init (&tp[i].mutex, 0);
		pthread_cond_init (&tp[i].cond, 0);
		
		if (pthread_create(&tp[i].id, NULL, threadpool_wait, &tp[i].index) != 0)
		{
			printf("pthread_create failed\n");
			return 0;
		}
	}
}

/*
void fuc(void *arg)
{
	printf("arg = %s\tthread = %lu 睡眠\r\n", arg, pthread_self());
}

int main(int argc, char *argv[])
{
	int i;



	sleep(2);
	
	free_thread();
	free_thread();
	free_thread();
	free_thread();
	sleep(3);
	return 1;
}
*/

