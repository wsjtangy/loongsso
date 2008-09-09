#include <time.h>
#include <stdio.h>
#include <tcadb.h>
#include <tcutil.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>

uint64_t ident_key();

unsigned int strhash(const char *str);

int main(int argc, char *argv[])
{
	int rc;
	char *val;
	char str[30];
	
	bool ret;
	uint64_t id;
	uint64_t re;
	unsigned int num;
	TCHDB *loong_info;
	
	char username[] = "lijinxing";
//	id  = 1210519264165944251LL;
	
	memset(&str, 0, sizeof(str));

	id  = ident_key();
	
	snprintf(str, sizeof(str), "%llu", id);

	num	= strhash(str);

	printf("num = %u\r\nmod = %u\r\nstr = %s\r\n", num, num % 10, str);
	
	unlink("test.db");
	loong_info = tchdbnew();
	if(!tchdbopen(loong_info, "test.db", HDBOWRITER | HDBOCREAT))
	{
		rc = tchdbecode(loong_info);
		printf("loong_info.db open error: %s\r\n", tchdberrmsg(rc));
		return 0;
	}

	if(!tchdbput(loong_info, username, strlen(username), str, strlen(str)))
	{
		rc = tchdbecode(loong_info);
		printf("loong_user error: %s\r\n", tchdberrmsg(rc));
	}

	if(!tchdbput(loong_info, (char *)&(id), sizeof(uint64_t), str, strlen(str)))
	{
		rc = tchdbecode(loong_info);
		printf("loong_user error: %s\r\n", tchdberrmsg(rc));
	}

	ret = tchdbout(loong_info, (char *)&(id), sizeof(uint64_t));
	if(ret)
	{
		printf("uint64_tÉ¾³ý³É¹¦\r\n");
	}
	else
	{
		printf("uint64_tÉ¾³ýÊ§°Ü\r\n");
	}


	val = tchdbget(loong_info, (char *)&(id), sizeof(uint64_t), &rc);
	if(val == NULL)
	{
		printf("id Ã»ÕÒµ½\r\n");
	}
	else
	{
		printf("id val = %s\r\n", val);
		free(val);
	}
	
	ret = tchdbout(loong_info, username, strlen(username));
	if(ret)
	{
		printf("stringÉ¾³ý³É¹¦\r\n");
	}
	else
	{
		printf("stringÉ¾³ýÊ§°Ü\r\n");
	}

	val = tchdbget2(loong_info, username);

	if(val == NULL)
	{
		printf("Ã»ÕÒµ½\r\n");
	}
	else
	{
		printf("val = %s\r\n", val);
		free(val);
	}

	tchdbclose(loong_info);
	tchdbdel(loong_info);

	return 0;
}

uint64_t ident_key()
{
	uint64_t id;
	char result[22];
	struct timespec resolution; 
	
	clock_gettime(CLOCK_REALTIME, &resolution);
		
	memset(result, 0, sizeof(result));
	snprintf(result, sizeof(result), "%ld%ld", resolution.tv_sec, resolution.tv_nsec);

	id = strtoull(result, 0, 10);
	return id;
}


unsigned int strhash(const char *str)
{
    int c;
    int hash = 5381;
    while (c = *str++)
	{
        hash = hash * 33 + c;
    }
	return hash == 0 ? 1 : hash;
}

