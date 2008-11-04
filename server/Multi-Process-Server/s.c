#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define NUM  4

volatile sig_atomic_t signal_exitc = 1;

void sigterm_handler (int sig)
{
	signal_exitc = 0;
	_exit(0);
}

int main(int argc, char *argv[])
{
	int    i;
	pid_t  pid[NUM];
	
	signal(SIGCLD, SIG_IGN);

	for(i=0; i<NUM; i++)
	{
		pid[i] = fork();
		if(pid[i] == 0)
		{
			struct sigaction sigterm;

			sigterm.sa_handler = sigterm_handler;

			if (sigaction(SIGTERM, &sigterm, NULL) < 0)
			{
				perror("unable to setup signal handler for SIGTERM");
				_exit(2);
			}
			while (signal_exitc)
			{
				printf("子进程pid = %lu\r\n", getpid());
				sleep(2);
			}
		}
	}
	
	sleep(4);
	for(i=0; i<NUM; i++)
	{
		printf("父进程pid = %lu\r\n", pid[i]);
		kill(pid[i], SIGTERM);
	}

	sleep(30);
	return 0;
}


