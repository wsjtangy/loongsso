#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
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


bool is_password(const char *str)
{
	int len, i;

	len = strlen(str);
	if(len < 6 || len > 20) return false;

	for(i=0; i<len; i++)
	{
		if(!isalnum(*(str+i))) return false;
	}
	return true;
}

//只包含英文字母和数字的用户名
bool is_alpha_username(const char *str)
{
	int len, i;

	len = strlen(str);
	for(i=0; i<len; i++)
	{
		if(!isalnum(*(str+i))) return false;
	}

	return true;
}

//只包含英文字母、数字、中文的用户名
bool is_ch_username(unsigned char *str)
{
	int len, i;
	
	len = strlen(str);
	if(len < 3 || len > 20) return false;
	
	for(i=0; i<len; i++)
	{
		if(*(str+i) >= 0x80)
		{
			i++;
		}
		else if(!isalnum(*(str+i)))
		{
			return false;
		}
	}

	return true;
}

int make_socket() 
{
	struct sockaddr_in addr;
	struct linger ling = {0, 0};	
	int sock, reuse_addr = 1;
	

	sock = socket(AF_INET, SOCK_STREAM, 0);    
    if (sock == -1)
    {
        perror("socket error :");
        exit(EXIT_FAILURE);
    }
    
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &reuse_addr, sizeof(flags));
	setsockopt(sock, SOL_SOCKET, SO_LINGER,    &ling,       sizeof(ling));


    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(conf.server_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
		perror("bind error :");
		exit(EXIT_FAILURE);
    }

	listen(sock, SOMAXCONN);

	return sock;
}

int set_nonblocking(int sock)
{
	int flags = 1;

	if (ioctl(sock, FIONBIO, &flags) && ((flags = fcntl(sock, F_GETFL, 0)) < 0 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)) 
	{
		return 0;
	}
	return 1;
}


bool is_mail(const char *str)
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
			return false;
		}
	}
	
	len = strlen(str);
	if(len > 29)
	{
		//email地址 不能超出30个字符
		return false;
	}
	
	for(i=0; i<len; i++)
	{
		if(str[i] == '@') break;
		if(!isalnum(str[i]) && str[i] != '.' && str[i] != '_' && str[i] != '-')
		{
			return false;
		}
	}
	return true;
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
					c = (strchr(recs[i].update, '?') == NULL) ? '?' : '&';
					es_append(res, recs[i].update);
					es_appendchar(res, c);
					es_append(res, json);
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

bool is_mail_exists(const char *str)
{
	char *val;
	
	val = tchdbget2(loong_mail, str);
	if(val == NULL)
	{
		return false;
	}
	
	free(val);
	return true;
}

bool is_user_exists(const char *str)
{
	char *val;
	
	val = tchdbget2(loong_user, str);
	if(val == NULL)
	{
		return false;
	}
	
	free(val);
	return true;
}

bool is_timeout(time_t t1, int minute)
{
	int n;
	time_t now;

	now = time((time_t*)0);
	n   = (int)difftime(now, t1);
	n  /= 60;   //转换为分钟
	
//	printf("n = %d\r\n", n, );
	if(n > minute)
	{
		//超时,返回假
		return false;
	}
	return true;
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
	
	data = tcmapnew2(15);
	
	tcmapput2(data, "uid",       uid);
	tcmapput2(data, "old_name",  rows[0]);
	tcmapput2(data, "password",  rows[1]);
	tcmapput2(data, "old_email", rows[2]);
	tcmapput2(data, "ip",        rows[3]);
	tcmapput2(data, "sex",       rows[4]);
	tcmapput2(data, "reg_time",  rows[5]);
	tcmapput2(data, "c_status",  rows[6]);

	mysql_free_result(res); 

	return data;
}

//更新用户数据
http_response_t update_user_info(TCMAP *data)
{
	uint64_t id;
	char code[34];
	char parm[256];
	unsigned int ind;
	my_ulonglong rows;
	int  parm_len, rc;
	struct loong_passwd info;
	const char *old_name,  *new_name;
	const char *old_email, *new_email;
	const char *password,  *uid,  *now, *ip;
	
	memset(&code, 0, sizeof(code));
	memset(&parm, 0, sizeof(parm));
	
	
	ip         = tcmapget2(data, "ip");
	uid        = tcmapget2(data, "uid");
	now        = tcmapget2(data, "reg_time");
	password   = tcmapget2(data, "password");
	new_name   = tcmapget2(data, "new_name");
	old_name   = tcmapget2(data, "old_name");
	new_email  = tcmapget2(data, "new_email");
	old_email  = tcmapget2(data, "old_email");

//CHINA_USERNAME  允许带中文、字母、数字的用户名
//ALPHA_USERNAME  允许带字母、数字的用户名

#ifdef CHINA_USERNAME
	if(new_name == NULL || !is_ch_username((unsigned char *)new_name))
#elif ALPHA_USERNAME
	if(new_name == NULL || !is_alpha_username(new_name))
#endif
	{
		//用户验证失败
		return HTTP_RESPONSE_USERNAME_NO;
	}
	else if( (strcasecmp(new_name, old_name) != 0) && is_user_exists(new_name))
	{
		//用户名已存在
		//当新用户名和旧用户名不一样的时候,才检查该用户名是否已存在
		return HTTP_RESPONSE_USERNAME_EXISTS;
	}
	else if(password == NULL || !is_password(password))
	{
		//密码验证失败
		return HTTP_RESPONSE_PASSWORD_NO;
	}
	else if(new_email == NULL || !is_mail(new_email))
	{
		//mail验证失败
		return HTTP_RESPONSE_EMAIL_NO;
	}
	else if((strcasecmp(new_email, old_email) != 0) && is_mail_exists(new_email))
	{
		//mail已存在
		return HTTP_RESPONSE_EMAIL_EXISTS;
	}

	
	MD5String((char *)password, code);

	id        = strtoull(uid, 0, 10);
	ind       = strhash(uid);
	parm_len  = snprintf(parm, sizeof(parm), "UPDATE member_%u SET `username` = '%s', `password` = '%s', `email` = '%s' WHERE `uid` = '%s'", (ind % TABLE_CHUNK), new_name, code, new_email, uid);
	rc        = mysql_real_query(dbh, parm, parm_len);
	if(rc)
	{
		//printf("Error making query: %s\n", mysql_error(dbh));
		return HTTP_RESPONSE_DB_ERROR;
	}
	//没更新到指定的uid,就直接返回了
	rows = mysql_affected_rows(dbh);
	if(rows < 1) return HTTP_RESPONSE_USERNAME_NOT_EXISTS;
	
	memset(&parm, 0, sizeof(parm));
	parm_len  = snprintf(parm, sizeof(parm), "id:%llu|username:%s|mail:%s|ip:%u|time:%u|", id, new_name, new_email, ip, now);
	
	info.id           = id;
	info.loong_status = 1;
	memcpy(info.pass, code, sizeof(info.pass));

	if(!tchdbput(loong_user, new_name, strlen(new_name), (char *)&(info), sizeof(struct loong_passwd)))
	{
		rc = tchdbecode(loong_user);
		//printf("loong_user error: %s\r\n", tchdberrmsg(rc));
		return HTTP_RESPONSE_CACHE_ERROR;
	}
	if(!tchdbput(loong_mail, new_email, strlen(new_email), (char *)&(info), sizeof(struct loong_passwd)))
	{
		rc = tchdbecode(loong_mail);
		//printf("loong_mail error: %s\r\n", tchdberrmsg(rc));
		return HTTP_RESPONSE_CACHE_ERROR;
	}
	if(!tchdbput(loong_info, (char *)&(id), sizeof(uint64_t), parm, parm_len))
	{
		rc = tchdbecode(loong_info);
		//printf("loong_info error: %s\r\n", tchdberrmsg(rc));
		return HTTP_RESPONSE_CACHE_ERROR;
	}

	tcmapdel(data);
	return HTTP_RESPONSE_UPDATE_OK;
}

//删除用户数据
http_response_t delete_user_info(TCMAP *data)
{
	uint64_t id;
	char query[128];
	unsigned int ind;
	my_ulonglong rows;
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
		return HTTP_RESPONSE_DB_ERROR;
	}
	
	//没删除到指定的uid,就直接返回了
	rows = mysql_affected_rows(dbh);
	if(rows < 1) return HTTP_RESPONSE_USERNAME_NOT_EXISTS;

	//删除缓存文件的用户信息
	tchdbout(loong_user, username, strlen(username));
	tchdbout(loong_mail, email, strlen(email));
	tchdbout(loong_info, (char *)&(id), sizeof(uint64_t));

	tcmapdel(data);
	return HTTP_RESPONSE_DELETE_OK;
}

