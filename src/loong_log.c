#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <loong.h>

FILE *fp;

void lonng_log_init()
{
	fp = fopen(LOONG_LOG_PATH, "a");
	if(fp == NULL)
	{
		printf("open(%s loong sso log file): %s\r\n", LOONG_LOG_PATH, strerror(errno));
		exit(0);
	}
}

void loong_write_log(const char *arg, ...)
{
	va_list ap;
	
	va_start(ap, arg);

	vfprintf(fp, arg, ap);
	va_end(ap);
}

void loong_destroy_log()
{
	fclose(fp);
}
