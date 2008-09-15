#include <stdio.h>
#include <stdarg.h>

FILE *fp;

void lonng_log_init()
{
	fp = fopen(LOONG_LOG_PATH, "a");
}

void loong_write_log()
{
}

void loong_destroy_log()
{
	fclose(fp);
}
