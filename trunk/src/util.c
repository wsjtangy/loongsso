#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <loong.h>

int MD5String(char *str, char *hex_output)
{	
	md5_state_t state;
	md5_byte_t digest[16];
	int di;
	
	if(str == NULL) return 0;

	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)str, strlen(str));
	md5_finish(&state, digest);
	
	for (di = 0; di < 16; ++di)
	{
		sprintf(hex_output + di * 2, "%02x", digest[di]);
	}

	return 1;
}


int is_password(char *str)
{
	int len, i;

	len = strlen(str);
	if(len < 6 || len > 20)
	{
		return 0;
	}

	for(i=0; i<len; i++)
	{
		if(!isalnum(str[i])) return 0;
	}
	return 1;
}

int is_username(unsigned char *str)
{
	int len;
	unsigned char *temp = str;
	
	len = strlen(str);

	while(*temp != '\0')
	{
		if(*temp >= 0xa1)
		{
			temp++;
		}
		else if(!isalnum(*temp))
		{
			return 0;
		}
		temp++;
	}

	if(len < 3 || len > 20)
	{
		return 0;
	}
	return 1;
}

int is_mail(char *str)
{
	int len, i;
	char *match;

	match = strchr(str, '@');
	if(match == NULL) return 0;
	
	match += 1;
	len    = strlen(match);
	for(i=0; i<len; i++)
	{
		if(!isalnum(match[i]) && match[i] != '.' && match[i] != '_' && match[i] != '-')
		{
			return 0;
		}
	}
	
	len = strlen(str);
	if(len > 29)
	{
		//email地址 不能超出30个字符
		return 0;
	}
	
	for(i=0; i<len; i++)
	{
		if(str[i] == '@') break;
		if(!isalnum(str[i]) && str[i] != '.' && str[i] != '_' && str[i] != '-')
		{
			return 0;
		}
	}
	return 1;
}


//生成验证码的 随机号ID
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



//生成验证码的 随机号值
void ident_value(unsigned char *result)
{
	int len, i, pos;
	char str[] = "3456789qwertyuipkjhgfdsaxcvbnmQWERTYUIPKJHGFDSAXCVBNM";
	
	len = strlen(str);

	for(i=0; i<4; i++)
	{
		pos       = rand() % len;
		result[i] = str[pos];
	}
}


char *long2ip(unsigned int v) 
{ 
	struct in_addr x; 
	
	x.s_addr = htonl(v); // passe dans l' ordre reseau 
	return inet_ntoa(x); // et convertit 
} 
  
unsigned int ip2long(char *s) 
{ 
	struct sockaddr_in n; 

	inet_aton(s,&n.sin_addr); 
	return ntohl(n.sin_addr.s_addr); 
}



void send_response(loong_conn *conn, http_response_t cmd, char *json)
{
	char c;
	char now[15];
	char sign[34];
	char url[500];
	char body[100];
	char header[800];
	char timebuf[100];
	int  header_len, body_len, i, err;

	estring res;
	char    *type;
	struct iovec vectors[2];
	struct loong_site *recs;

	memset(now, 0, sizeof(now));
	memset(url, 0, sizeof(url));
	memset(body, 0, sizeof(body));
	memset(timebuf, 0, sizeof(timebuf));
	
	snprintf(now, sizeof(now), "%d", conn->now);
	body_len   = snprintf(body, sizeof(body), "loong_status = %d;", cmd);
	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", localtime(&conn->now));

	if(cmd == HTTP_RESPONSE_LOGIN_OK || cmd == HTTP_RESPONSE_REGISTER_OK)
	{
		res  = es_init();
		recs = conf.site;
		
		type = cmd == HTTP_RESPONSE_LOGIN_OK ? "login" : "register";
		urlencode(json, url, sizeof(url));

		for(i=0; i<conf.num; i++)
		{
			memset(sign,   0, sizeof(sign));
			memset(header, 0, sizeof(header));

			snprintf(header, sizeof(header), "%s|%s|%s", recs[i].key, json, now);
			
			MD5String(header, sign);

			es_append(res, "load_script(\"");
			es_append(res, recs[i].login);
			if(strchr(recs[i].login, '?') == NULL)
			{
				es_appendchar(res, '?');
			}
			else
			{
				es_appendchar(res, '&');
			}

			es_append(res, "loong_type=");
			es_append(res, type);
			es_append(res, "&loong_info=");
			es_append(res, url);
			es_append(res, "&loong_time=");
			es_append(res, now);
			es_append(res, "&loong_sign=");
			es_append(res, sign);
			es_append(res, "\");\r\n");
		}
		
		es_append(res, body);

		memset(header, 0, sizeof(header));
		header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: loongSSO(RC1.0)\r\nContent-Length: %u\r\nConnection: close\r\nContent-Type: application/x-javascript\r\n\r\n", timebuf, es_len(res));

		vectors[0].iov_base = header;
		vectors[0].iov_len  = header_len;
		vectors[1].iov_base = es_get(res);
		vectors[1].iov_len  = es_len(res);
		
		writev(conn->sfd, vectors, 2);
		es_free(res);
	}
	else if(cmd == HTTP_RESPONSE_LOGOUT_OK || cmd == HTTP_RESPONSE_DELETE_OK || cmd == HTTP_RESPONSE_UPDATE_OK)
	{
		res  = es_init();
		recs = conf.site;

		for(i=0; i<conf.num; i++)
		{
			es_append(res, "load_script(\"");

			switch(cmd)
			{
				case HTTP_RESPONSE_LOGOUT_OK:
					es_append(res, recs[i].logout);
					break;
				case HTTP_RESPONSE_DELETE_OK:
					c = (strchr(recs[i].del, '?') == NULL) ? '?' : '&';
					es_append(res, recs[i].del);
					es_appendchar(res, c);
					es_append(res, "uid=");
					es_append(res, json);
					break;
				case HTTP_RESPONSE_UPDATE_OK:
					es_append(res, recs[i].update);
					break;
			}

			es_append(res, "\");\r\n");
		}
		
		es_append(res, body);

		memset(header, 0, sizeof(header));
		header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: loongSSO(RC1.0)\r\nContent-Length: %u\r\nConnection: close\r\nContent-Type: application/x-javascript\r\n\r\n", timebuf, es_len(res));

		vectors[0].iov_base = header;
		vectors[0].iov_len  = header_len;
		vectors[1].iov_base = es_get(res);
		vectors[1].iov_len  = es_len(res);
		
		writev(conn->sfd, vectors, 2);
		es_free(res);
	}
	else
	{
		memset(header, 0, sizeof(header));

		header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: loongSSO(RC1.0)\r\nContent-Length: %u\r\nConnection: close\r\nContent-Type: application/x-javascript\r\n\r\n", timebuf, body_len);

		vectors[0].iov_base = header;
		vectors[0].iov_len  = header_len;
		vectors[1].iov_base = body;
		vectors[1].iov_len  = body_len;

		writev(conn->sfd, vectors, 2);
	}
}

int is_mail_exists(char *str)
{
	char *val;

	if(!(val = tchdbget2(loong_mail, str)))
	{
		//没找到
		return 1;
	}

	free(val);
	return 0;
}

int is_user_exists(char *str)
{
	char *val;

	if(!(val = tchdbget2(loong_user, str)))
	{
		//没找到
		return 1;
	}

    free(val);
	
	//找到用户名
	return 0;
}

unsigned long urlencode(unsigned char   *csource, unsigned char   *cbuffer, unsigned long   lbuffersize)
{
    unsigned long   llength;                                        
    unsigned long   lcount = 0;                                    
    unsigned char   cbyte;                                         
    unsigned char   ctemp[4];                                       
    unsigned long   lresultcount = 0;                             

    llength = (unsigned long)strlen(csource);                      
    if(!llength) { return lresultcount; }                           
    if(lbuffersize < (llength * 3 + 1)) { return lresultcount; }

    while(1) {
        cbyte = *(csource + lcount);                                
        if( ((cbyte >= 0x81) && (cbyte <= 0x9F)) ||
            ((cbyte >= 0xE0) && (cbyte <= 0xEF)) ) {              
            sprintf(ctemp, "%%%02X", cbyte);                       
            strncpy(cbuffer + lresultcount, ctemp, 4);            
            lcount++;                                             
            lresultcount += 3;                                    
            if(lcount == llength) { break; }                        
            sprintf(ctemp, "%%%02X", *(csource + lcount));         
            strncpy(cbuffer + lresultcount, ctemp, 4);              
            lcount++;                                           
            lresultcount += 3;                                      
        } else if(cbyte == 0x20) {                                  
            strncpy(cbuffer + lresultcount, "+", 2);             
            lcount++;                                               
            lresultcount++;                                        
        } else if( ((cbyte >= 0x40) && (cbyte <= 0x5A)) ||         
                   ((cbyte >= 0x61) && (cbyte <= 0x7A)) ||          
                   ((cbyte >= 0x30) && (cbyte <= 0x39)) ||          
                   (cbyte == 0x2A) ||                             
                   (cbyte == 0x2D) ||                            
                   (cbyte == 0x2E) ||                           
                   (cbyte == 0x5F) ) {                          
            strncpy(cbuffer + lresultcount, csource + lcount, 2);
            lcount++;                                               
            lresultcount++;                                       
        } else {                                                
            sprintf(ctemp, "%%%02X", cbyte);                     
            strncpy(cbuffer + lresultcount, ctemp, 4);            
            lcount++;                                               
            lresultcount += 3;                             
        }
        if(lcount == llength) { break; }                            
    }
    return lresultcount;                                            
}


int daemon(int nochdir, int noclose)
{
    int fd;

    switch (fork()) 
	{
		case -1:
			return (-1);
		case 0:
			break;
		default:
			_exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return (-1);

    if (nochdir == 0)
        (void)chdir("/");

    if (noclose==0 && (fd = open("/dev/null", O_RDWR, 0)) != -1) 
	{
        (void)dup2(fd, STDIN_FILENO);
        (void)dup2(fd, STDOUT_FILENO);
        (void)dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            (void)close(fd);
    }
    return (0);
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

TCMAP *fetch_user_info(const char *uid)
{
	TCMAP *data;
	MYSQL_RES *res; 
	MYSQL_ROW rows; 
	unsigned int  ind;
	unsigned long num; 
	int  query_len, rc;
	char query[256] = {0};

	const char *key;
	
	ind       = strhash(uid);
	query_len = snprintf(query, sizeof(query), "SELECT `username`, `password`, `email`, `ip`, `sex`, `reg_time`, `c_status` FROM member_%d WHERE `uid` = '%s'", (ind % TABLE_CHUNK), uid);
	
	rc   = mysql_real_query(dbh, query, query_len);
	if(rc) return NULL;
	
	res  = mysql_store_result(dbh); 
	if(res == NULL) return NULL;
	
	num  = mysql_num_rows(res); 

	if(num == 0)
	{
		mysql_free_result(res); 
		return NULL;
	}
	
	rows = mysql_fetch_row(res);
	
	data = tcmapnew();
	
	tcmapput2(data, "uid",      uid);
	tcmapput2(data, "username", rows[0]);
	tcmapput2(data, "password", rows[1]);
	tcmapput2(data, "email",    rows[2]);
	tcmapput2(data, "ip",       rows[3]);
	tcmapput2(data, "sex",      rows[4]);
	tcmapput2(data, "reg_time", rows[5]);
	tcmapput2(data, "c_status", rows[6]);

	mysql_free_result(res); 

	return data;
}

//更新用户数据
int update_user_info(TCMAP *data)
{
}

//删除用户数据
int delete_user_info(TCMAP *data)
{
	uint64_t id;
	char query[128];
	unsigned int  ind;
	int  query_len, rc;
	const char *username, *email, *uid;
	
	memset(&query, 0, sizeof(query));
	uid      = tcmapget2(data, "uid");
	email    = tcmapget2(data, "email");
	username = tcmapget2(data, "username");
	
	ind       = strhash(uid);
	id        = strtoull(uid, 0, 10);
	query_len = snprintf(query, sizeof(query), "DELETE FROM member_%d WHERE `uid` = '%s'", (ind % TABLE_CHUNK), uid);
	rc        = mysql_real_query(dbh, query, query_len);
	if(rc)
	{
		//printf("Error making query: %s\n", mysql_error(dbh));
		return 0;
	}

	//删除缓存文件的用户信息
	tchdbout(loong_user, username, strlen(username));
	tchdbout(loong_mail, email, strlen(email));
	tchdbout(loong_info, (char *)&(id), sizeof(uint64_t));

	tcmapdel(data);
	return 1;
}

