#include <time.h>
#include <stdio.h>
#include <tcadb.h>
#include <tcutil.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <mysql.h>

// gcc -o t t.c -I/home/lzy/local/tokyocabinet/include -I/usr/include/mysql/ -L/usr/lib/mysql/ -L/home/lzy/local/tokyocabinet/lib -ltokyocabinet -lmysqlclient -lrt


uint64_t ident_key();

TCMAP *fetch_user_info(char *uid);

unsigned int strhash(const char *str);

void Test_out();

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

	val = tchdbget2(loong_info, username);

	if(val == NULL)
	{
		printf("没找到\r\n");
	}
	else
	{
		printf("val = %s\r\n", val);
		free(val);
	}

	if(!tchdbput(loong_info, username, strlen(username), "李锦星", strlen("李锦星")))
	{
		rc = tchdbecode(loong_info);
		printf("loong_user error: %s\r\n", tchdberrmsg(rc));
	}


	val = tchdbget2(loong_info, username);

	if(val == NULL)
	{
		printf("没找到\r\n");
	}
	else
	{
		printf("val = %s\r\n", val);
		free(val);
	}

/*
	ret = tchdbout(loong_info, (char *)&(id), sizeof(uint64_t));
	if(ret)
	{
		printf("uint64_t删除成功\r\n");
	}
	else
	{
		printf("uint64_t删除失败\r\n");
	}


	val = tchdbget(loong_info, (char *)&(id), sizeof(uint64_t), &rc);
	if(val == NULL)
	{
		printf("id 没找到\r\n");
	}
	else
	{
		printf("id val = %s\r\n", val);
		free(val);
	}
	
	ret = tchdbout(loong_info, username, strlen(username));
	if(ret)
	{
		printf("string删除成功\r\n");
	}
	else
	{
		printf("string删除失败\r\n");
	}

	val = tchdbget2(loong_info, username);

	if(val == NULL)
	{
		printf("没找到\r\n");
	}
	else
	{
		printf("val = %s\r\n", val);
		free(val);
	}

*/
	tchdbclose(loong_info);
	tchdbdel(loong_info);
	
	Test_out();

//	fetch_user_info("1220582278313757000");
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

int is_timeout(time_t t1, int minute)
{
	int n;
	time_t now;

	now = time((time_t*)0);
	n   = (int)difftime(now, t1);
	n  /= 60;   //转换为分钟
	
	printf("n = %d\tnow = %u\r\n", n, now);
	if(n > minute)
	{
		//超时,返回假
		return 0;
	}
	return 1;
}


void Test_out()
{
	size_t sec;
	char now[] = "1221121235";

	sec = (time_t)strtol(now, 0, 10);
	
	is_timeout(sec, 3);
	printf("sec = %u\tnow = %s\r\n", sec, now);
}


/*
TCMAP *fetch_user_info(char *uid)
{
	MYSQL *conn;
	TCMAP *data;
	MYSQL_RES *res; 
	MYSQL_ROW rows; 
	unsigned int  ind;
	unsigned long num; 
	int  query_len, rc;
	char query[256] = {0};

	const char *key;
	
//	query_len = snprintf(query, sizeof(query), "SELECT `username`, `password`, `email`, `ip`, `sex`, `reg_time`, `c_status` FROM member_%d WHERE `uid` = '%s'", (ind % TABLE_CHUNK), uid);

	query_len = snprintf(query, sizeof(query), "SELECT `username`, `password`, `email`, `ip`, `sex`, `reg_time`, `c_status` FROM member WHERE `uid` = '%s'", uid);
	conn  = mysql_init(NULL);
    if (conn == NULL)
    {
        printf("mysql_init() failed (probably out of memory)\r\n");
        return NULL;
    }
	if (mysql_real_connect (conn, "122.11.49.209", "qiye", "lijinxing_qiye_loongsso", "loongsso", 3306, NULL, CLIENT_MULTI_STATEMENTS) == NULL)
    {
        printf ("mysql_real_connect() failed\r\n");
        mysql_close (conn);
        return 0;
    }

	rc   = mysql_real_query(conn, query, query_len);
	if(rc) return NULL;

	res  = mysql_store_result(conn); 
	if(res == NULL) return NULL;

	num  = mysql_num_rows(res); 
	if(num == 0)
	{
		mysql_free_result(res); 
		return NULL;
	}
	
	rows = mysql_fetch_row(res);
	
	data = tcmapnew();
	tcmapput2(data, "username", rows[0]);
	tcmapput2(data, "password", rows[1]);
	tcmapput2(data, "email",    rows[2]);
	tcmapput2(data, "ip",       rows[3]);
	tcmapput2(data, "sex",      rows[4]);
	tcmapput2(data, "reg_time", rows[5]);
	tcmapput2(data, "c_status", rows[6]);

	mysql_free_result(res); 

	mysql_close (conn);

	return data;
}
*/
