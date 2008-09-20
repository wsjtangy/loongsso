#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <loong.h>

void decodevalue(const char *s)
{
	char *s_ptr;
	char *decoded_ptr;
	char hex_str[3] = "\0\0\0";
	unsigned int hex_val;

	s_ptr = (char *) s;
	decoded_ptr = (char *) s;

	while (*s_ptr != '\0')
	{
		if (*s_ptr == '+')
		{
			*decoded_ptr = ' ';
		}
		else if (*s_ptr == '%')
		{
			hex_str[0] = *(++s_ptr);
			hex_str[1] = *(++s_ptr);
			sscanf(hex_str, "%x", &hex_val);
			*decoded_ptr = (char) hex_val;
		}
		else
		{
			*decoded_ptr = *s_ptr;
		}
			
		s_ptr++;
		decoded_ptr++;
	}

	*decoded_ptr = '\0';
}


char *nexttoken(char *s, char separator)
{
	static char *p;
	static int at_the_end = 0;
	char *start_position;

	if (s == NULL)    /* not the first call for this string */
	{
		if (at_the_end)
			return NULL;
	}
	else              /* is the first call for this string */
	{
		p = s;
		at_the_end = 0;
	}

	start_position = p;

	while (*p)
	{
		if (*p == separator)
		{
			*p = '\0';
			p++;
			return start_position;

		}
		p++;
	}
	at_the_end = 1;
	return start_position;
}

//解析uri参数
int parse_get(char *form_data, loong_conn *conn)
{
	char *name, *value;

	if ((name = nexttoken(form_data, '=')) == NULL)
		return 0;
	if ((value = nexttoken(NULL, '&')) == NULL)
		return 0;
	decodevalue(value);
	
	if(strlen(name) > 0 && strlen(value) > 0)
	{
		tcmapput2(conn->recs, name, value);
	//	printf("%s => %s\r\n", name, value);
	}

	while (1)
	{
		if ((name = nexttoken(NULL, '=')) == NULL)
			break;
		if ((value = nexttoken(NULL, '&')) == NULL)
			break;
		decodevalue(value);
		
		if(strlen(name) > 0 && strlen(value) > 0)
		{
			tcmapput2(conn->recs, name, value);
		//	printf("%s => %s\r\n", name, value);
		}
	}

	return 1;
}



//0 解析失败
//1 解析成功
int parse_http_header(char *header, size_t header_len, loong_conn *conn)
{
	char *ptr, *cookie;
	int i, pos, j, len;
	
	char buff[DATA_BUFFER_SIZE], uri[DATA_BUFFER_SIZE];
	
	
	pos = 0;
	ptr = header;
	
	memset(uri, 0, sizeof(uri));
	memset(buff, 0, sizeof(buff));

	for(i=0; i<header_len; i++)
	{
		if(*(ptr+i) == '\r' || *(ptr+i) == '\n')
		{
			len = strlen(buff);
			if(strncasecmp(buff, "GET /?", 6) == 0)
			{
				pos = 0;
				for(i=6; i<len; i++)
				{
					if(isspace(buff[i])) break;
					
					if(pos >= DATA_BUFFER_SIZE) return 0;
					uri[pos++] = buff[i];
				}
				parse_get(uri, conn);
			}
			else if(strncasecmp(buff, "Cookie: ", 8) == 0)
			{
				cookie     = buff + 8;
				cookie     = strstr(cookie, "loongSSO=");
				if(cookie != NULL)
				{
					conn->code = strtoull(cookie + 9, 0, 10);
					//memcpy(conn->cookie, cookie + 9, sizeof(conn->cookie));
				}
			}

			memset(buff, 0, sizeof(buff));
			pos = 0;
		}
		else
		{
			if(pos >= DATA_BUFFER_SIZE) return 0;
			buff[pos++] = *(ptr+i);
		}
	}

	return 1;
}

