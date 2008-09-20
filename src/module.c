#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <gd.h>
#include <gdfontg.h>
#include <gdfonts.h>
#include <gdfontl.h>
#include <loong.h>


//生成验证码
int loong_sso_validate(loong_conn *conn)
{
	uint64_t   key;
	gdImagePtr img;
	char header[700], value[5];
	
	time_t lifetime;
	unsigned char *data;
	struct iovec vectors[2];
	char timebuf[100], cookie_buf[100];

	int watermark, background, font_color, randcolor, i, size, header_len, rc;
	
	srand((unsigned)time(0));

	memset(header, 0, sizeof(header));
	
	rc = 0;
	while(!rc)
	{
		//如果添加失败,就循环尝试添加数据。直到返回1(true)
		memset(value, 0, sizeof(value));

		key = ident_key();
		ident_value(value);

		lifetime = time((time_t*)0);
		rc = hash_add(codepool, key, value, lifetime);
	}
	
	lifetime += 60;

	img        = gdImageCreate(80, 25);  
	background = gdImageColorAllocate(img, 114 , 114 , 114); 
	watermark  = gdImageColorAllocate(img, 157 , 157 , 157); 
	font_color = gdImageColorAllocate(img , 254 , 248 , 248);

	gdImageFill(img, 0, 0, background);    
	
	for(i=0; i<200; i++)
	{ 
		// 加入干扰象素 
		randcolor = gdImageColorAllocate(img, rand() % 255, rand() % 255, rand() % 255);
		gdImageSetPixel(img, rand() % 80 , rand() % 25 , randcolor); 
	}
	
	gdImageString(img, gdFontLarge, 5, 5, WATER_MARK, watermark);
	
	gdImageStringFT(img, 0, font_color, SIGN_FONT_PATH, 15, 0.0, 10, 20, value);
	
	data = (unsigned char *)gdImagePngPtr(img, &size);

	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", localtime(&conn->now));
	strftime(cookie_buf, sizeof(cookie_buf), "%a, %d-%b-%Y %H:%M:%S GMT", localtime(&lifetime));

	header_len = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nDate: %s\r\nServer: loongSSO(RC1.0)\r\nContent-Length: %u\r\nConnection: close\r\nContent-Type: image/png\r\nP3P: CP=\"CURa ADMa DEVa PSAo PSDo OUR BUS UNI PUR INT DEM STA PRE COM NAV OTC NOI DSP COR\"\r\nSet-Cookie: loongSSO=%llu; expires=%s; domain=%s\r\n\r\n", timebuf, size, key, cookie_buf, conf.domain);
	
	vectors[0].iov_base = header;
    vectors[0].iov_len  = header_len;
    vectors[1].iov_base = data;
    vectors[1].iov_len  = size;

	writev(conn->sfd, vectors, 2);

	gdFree(data);

	return 0;
}

//loongSSO 注册
int loong_sso_register(loong_conn *conn)
{
	const char *k;
	const char *email;
	const char *username;
	const char *password;
	const char *public_code;
	const char *private_code;
	

	uint64_t id, key;
	unsigned int  ind;
	int json_len, query_len, rc;
	char json[256], query[512], code[34], tmep[30];
	
	struct loong_passwd info;

	email        = tcmapget2(conn->recs, "email");
	private_code = hash_get(codepool, conn->code);
	public_code  = tcmapget2(conn->recs, "code");
	username     = tcmapget2(conn->recs, "username");
	password     = tcmapget2(conn->recs, "password");


//CHINA_USERNAME  允许带中文、字母、数字的用户名
//ALPHA_USERNAME  允许带字母、数字的用户名

#ifdef CHINA_USERNAME
	if(username == NULL || !is_ch_username((unsigned char *)username))
#elif ALPHA_USERNAME
	if(username == NULL || !is_alpha_username(username))
#endif
	{
		//用户验证失败
		send_response(conn, HTTP_RESPONSE_USERNAME_NO, NULL);
		return 0;
	}
	else if(is_user_exists(username))
	{
		//用户名已存在
		send_response(conn, HTTP_RESPONSE_USERNAME_EXISTS, NULL);
		return 0;
	}
	else if(password == NULL || !is_password(password))
	{
		//密码验证失败
		send_response(conn, HTTP_RESPONSE_PASSWORD_NO, NULL);
		return 0;
	}
	else if(email == NULL || !is_mail(email))
	{
		//mail验证失败
		send_response(conn, HTTP_RESPONSE_EMAIL_NO, NULL);
		return 0;
	}
	else if(is_mail_exists(email))
	{
		//mail已存在
		send_response(conn, HTTP_RESPONSE_EMAIL_EXISTS, NULL);
		return 0;
	}
	else if( (private_code == NULL) || (public_code == NULL) || (strcasecmp(private_code, public_code) != 0) )
	{
		//验证失败
		send_response(conn, HTTP_RESPONSE_VALIDATE_NO, NULL);
		return 0;
	}
	
	//注册成功,在hash表里面删除 验证码数据
	hash_remove(codepool, conn->code);
	
	memset(code, 0, sizeof(code));
	memset(json, 0, sizeof(json));
	memset(tmep, 0, sizeof(tmep));
	memset(query, 0, sizeof(query));
	
	
	MD5String((char *)password, code);

	id        = ident_key();
	
	snprintf(tmep, sizeof(tmep), "%llu", id);

	ind       = strhash(tmep);
	json_len  = snprintf(json, sizeof(json), "id:%llu|username:%s|mail:%s|ip:%u|time:%u|", id, username, email, ip2long(conn->ip), conn->now);
	query_len = snprintf(query, sizeof(query), "INSERT INTO `member_%d` (`uid`, `username`, `password`, `email`, `ip`, `reg_time`, `c_status`)VALUES('%llu', '%s', '%s', '%s', %u, %u, 1);", (ind % TABLE_CHUNK), id, username, code, email, ip2long(conn->ip), conn->now);
	

	info.id           = id;
	info.loong_status = 1;
	memcpy(info.pass, code, sizeof(info.pass));

	rc = mysql_real_query(dbh, query, query_len);
	if(rc)
	{
		//数据库查询错误
		//printf("Error making query: %s\n", mysql_error(dbh));
		send_response(conn, HTTP_RESPONSE_DB_ERROR, json);
		return 1;
	}

	if(!tchdbput(loong_user, username, strlen(username), (char *)&(info), sizeof(struct loong_passwd)))
	{
		rc = tchdbecode(loong_user);
		//printf("loong_user error: %s\r\n", tchdberrmsg(rc));
	}
	if(!tchdbput(loong_mail, email, strlen(email), (char *)&(info), sizeof(struct loong_passwd)))
	{
		rc = tchdbecode(loong_mail);
		//printf("loong_mail error: %s\r\n", tchdberrmsg(rc));
	}
	if(!tchdbput(loong_info, (char *)&(id), sizeof(uint64_t), json, json_len))
	{
		rc = tchdbecode(loong_info);
		//printf("loong_info error: %s\r\n", tchdberrmsg(rc));
	}

	send_response(conn, HTTP_RESPONSE_REGISTER_OK, json);
	return 1;
}

//用户登陆
// /?module=login&type=mail&date=xxxx&password=xxxx
// /?module=login&type=user&date=xxxx&password=xxxx
int loong_sso_login(loong_conn *conn)
{
	int rc;
	char code[34];
	struct loong_passwd *info;
	char *val, *password, *json;
	const char *type, *data;
	
	memset(code, 0, sizeof(code));
	type     = tcmapget2(conn->recs, "type");
	data     = tcmapget2(conn->recs, "data");
	password = (char *)tcmapget2(conn->recs, "password");

	if(type == NULL)
	{
		//未知类型
		send_response(conn, HTTP_RESPONSE_UNKNOWN_TYPE, NULL);
		return 0;
	}
	else if(strcasecmp(type, "mail") == 0)
	{
		if((data == NULL) || !is_mail_exists(data))
		{
			//mail不存在
			send_response(conn, HTTP_RESPONSE_EMAIL_NO, NULL);
			return 0;
		}
	}
	else if(strcasecmp(type, "user") == 0)
	{
		if((data == NULL) || !is_user_exists(data))
		{
			//用户名不存在
			send_response(conn, HTTP_RESPONSE_USERNAME_NO, NULL);
			return 0;
		}
	}
	else
	{
		//未知类型
		send_response(conn, HTTP_RESPONSE_UNKNOWN_TYPE, NULL);
		return 0;
	}
	
	MD5String(password, code);
	info = (struct loong_passwd *)val;
	if( (password == NULL) || (strcasecmp(info->pass, code) != 0) )
	{
		//密码不匹配
		send_response(conn, HTTP_RESPONSE_PASSWORD_NO, NULL);
		free(val);
		return 0;
	}

	if(!(json = tchdbget(loong_info, (char *)&(info->id), sizeof(uint64_t), &rc)))
	{
		//printf("dpget: %s\n", dperrmsg(dpecode));
		//用户名不存在
		send_response(conn, HTTP_RESPONSE_USERNAME_NO, NULL);
		return 0;
	}
	
	//登陆成功
	send_response(conn, HTTP_RESPONSE_LOGIN_OK, json);
	free(val);
	free(json);
	return 1;
}

//用户验证
// /?module=check&type=mail&data=xxxx
// /?module=check&type=user&data=xxxx
int loong_sso_check(loong_conn *conn)
{
	const char *type, *data;

	type = tcmapget2(conn->recs, "type");
	data = tcmapget2(conn->recs, "data");
	
	if(type == NULL)
	{
		send_response(conn, HTTP_RESPONSE_UNKNOWN_TYPE, NULL);
	}
	else if(strcasecmp(type, "mail") == 0)
	{
		if(data == NULL || is_mail_exists(data))
		{
			send_response(conn, HTTP_RESPONSE_EMAIL_NO, NULL);
		}
		else
		{
			send_response(conn, HTTP_RESPONSE_EMAIL_OK, NULL);
		}
	}
	else if(strcasecmp(type, "user") == 0)
	{
		if(data == NULL || is_user_exists(data))
		{
			send_response(conn, HTTP_RESPONSE_USERNAME_NO, NULL);
		}
		else
		{
			send_response(conn, HTTP_RESPONSE_USERNAME_OK, NULL);
		}
	}
	else
	{
		send_response(conn, HTTP_RESPONSE_UNKNOWN_TYPE, NULL);
	}

	return 1;
}


//用户更新 用户信息
// /?module=update
int loong_sso_update(loong_conn *conn)
{
	int   i;
	time_t sec;
	TCMAP *data;
	char  code[35];
	char  str[300];
	http_response_t cmd;
	struct loong_site *recs;
	const char *mode, *uid, *sign, *username, *email, *password, *now;
	
	now      = tcmapget2(conn->recs, "now");
	uid      = tcmapget2(conn->recs, "uid");
	mode     = tcmapget2(conn->recs, "mode");
	sign     = tcmapget2(conn->recs, "sign");
	email    = tcmapget2(conn->recs, "email");
	username = tcmapget2(conn->recs, "username");
	password = tcmapget2(conn->recs, "password");

	if(uid == NULL || mode == NULL || sign == NULL || email == NULL || username == NULL || password == NULL || now == NULL)
	{
		send_response(conn, HTTP_RESPONSE_VARIABLE_ERROR, NULL);
		return 0;
	}

	sec = (time_t)strtol(now, 0, 10);
	i   = is_timeout(sec, conf.timeout);
	if(!i)
	{
		send_response(conn, HTTP_RESPONSE_TIMEOUT, NULL);
		return 0;
	}

	recs = conf.site;
	for(i=0; i<conf.num; i++)
	{
		if(strcasecmp(mode, recs[i].id) == 0)
		{
			memset(&str, 0, sizeof(str));
			memset(&code, 0, sizeof(code));
			
			snprintf(str, sizeof(str), "%s|%s|%s|%s|%s|%s", recs[i].update_key, uid, username, password, email, now);
			MD5String(str, code);
			
			if(strcasecmp(sign, code) == 0)
			{
				data = fetch_user_info(uid);
				if(data != NULL)
				{
					//在数据库里找到数据,并更新数据
					memset(&str, 0, sizeof(str));
					memset(&code, 0, sizeof(code));
					
					//为回调参数生成数字签名
					snprintf(str, sizeof(str), "%s|%s|%s|%u", uid, username, email, conn->now);
					MD5String(str, code);
					
					//生成回调的参数
					memset(&str, 0, sizeof(str));
					snprintf(str, sizeof(str), "uid=%s&username=%s&email=%s&date=%u&sign=%s",  uid, username, email, conn->now, code);
					
					//用最新的 覆盖旧有的数据
					tcmapput2(data, "new_name",  username);
					tcmapput2(data, "password",  password);
					tcmapput2(data, "new_email", email);
					
					cmd = update_user_info(data);
					send_response(conn, cmd, str);
				}
				else
				{
					//在数据库里没找到相应的uid数据
					send_response(conn, HTTP_RESPONSE_USERNAME_NOT_EXISTS, NULL);
				}
				
				return 1;
			}
			else
			{
				//数字签名匹配不上
				send_response(conn, HTTP_RESPONSE_SIGN_NO, NULL);
				return 1;
			}
		}
	}
	
	//在loong SSO的 站点列表 没找到相应的站点ID
	send_response(conn, HTTP_RESPONSE_SITE_NOT_EXISTS, NULL);
	return 1;
}

//删除用户信息
// /?module=delete
int loong_sso_delete(loong_conn *conn)
{
	int   i;
	time_t sec;
	TCMAP *data;
	char  code[35];
	char  str[100];
	http_response_t cmd;
	struct loong_site *recs;
	const char *mode, *uid, *sign, *now;
	
	uid  = tcmapget2(conn->recs, "uid");
	now  = tcmapget2(conn->recs, "now");
	mode = tcmapget2(conn->recs, "mode");
	sign = tcmapget2(conn->recs, "sign");

	if(uid == NULL || mode == NULL || sign == NULL || now == NULL)
	{
		send_response(conn, HTTP_RESPONSE_VARIABLE_ERROR, NULL);
		return 0;
	}
	
	sec = (time_t)strtol(now, 0, 10);
	i   = is_timeout(sec, conf.timeout);
	if(!i)
	{
		send_response(conn, HTTP_RESPONSE_TIMEOUT, NULL);
		return 0;
	}

	recs = conf.site;
	for(i=0; i<conf.num; i++)
	{
		if(strcasecmp(mode, recs[i].id) == 0)
		{
			memset(&str, 0, sizeof(str));
			memset(&code, 0, sizeof(code));
			
			snprintf(str, sizeof(str), "%s|%s|%s", recs[i].update_key, uid, now);
			MD5String(str, code);
			
			if(strcasecmp(sign, code) == 0)
			{
				data = fetch_user_info(uid);
				if(data != NULL)
				{
					//在数据库里找到数据,并删除数据
					cmd = delete_user_info(data);
					send_response(conn, cmd, (char *)uid);
				}
				else
				{
					//在数据库里没找到相应的uid数据
					send_response(conn, HTTP_RESPONSE_USERNAME_NOT_EXISTS, NULL);
				}
				
				return 1;
			}
			else
			{
				//数字签名匹配不上
				send_response(conn, HTTP_RESPONSE_SIGN_NO, NULL);
				return 1;
			}
		}
	}
	
	//在loong SSO的 站点列表 没找到相应的站点ID
	send_response(conn, HTTP_RESPONSE_SITE_NOT_EXISTS, NULL);
	return 1;
}


//用户退出
// /?module=logout
int loong_sso_logout(loong_conn *conn)
{
	send_response(conn, HTTP_RESPONSE_LOGOUT_OK, NULL);
	return 1;
}


