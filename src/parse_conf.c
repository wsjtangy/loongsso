#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <loong.h>



// private 解析根节点下的信息
void parse_root_info(sxml_node_t *root)
{
	const char * content;	
	sxml_node_t *node, *np;

	node = sxml_find_element(root, "loongSSO", NULL, NULL);
	if(node != NULL)
	{
		for (np = sxml_get_child(node); np != NULL; np = sxml_get_next_sibling(np))
		{
			if (sxml_get_type(np) != SXML_ELEMENT) { continue; }
			if (sxml_get_element_name(np) == NULL) { continue; }

			content = sxml_get_content(sxml_get_child(np));
			if(strcasecmp(sxml_get_element_name(np), "server_port") == 0)
			{
				conf.server_port = atoi(content);
			}
			if(strcasecmp(sxml_get_element_name(np), "server_id") == 0)
			{
				conf.server_id = atoi(content);
			}
			if(strcasecmp(sxml_get_element_name(np), "domain") == 0)
			{
				memcpy(conf.domain, content, sizeof(conf.domain));
			}
		}
	}
}

// private 解析站点列表
void parse_site_list(sxml_node_t *node)
{
	int pos;
	sxml_node_t *np;
	const char * content;	
	struct loong_site *recs;

	if(node != NULL)
	{
		pos  = conf.num;
		recs = conf.site;

		for (np = sxml_get_child(node); np != NULL; np = sxml_get_next_sibling(np))
		{
			if (sxml_get_type(np) != SXML_ELEMENT) { continue; }
			if (sxml_get_element_name(np) == NULL) { continue; }

			content = sxml_get_content(sxml_get_child(np));
			if(strcasecmp(sxml_get_element_name(np), "key") == 0)
			{
				memcpy(recs[pos].key, content, sizeof(recs[pos].key));
			}
			else if(strcasecmp(sxml_get_element_name(np), "update_key") == 0)
			{
				memcpy(recs[pos].update_key, content, sizeof(recs[pos].update_key));
			}
			else if(strcasecmp(sxml_get_element_name(np), "id") == 0)
			{
				memcpy(recs[pos].id, content, sizeof(recs[pos].id));
			}
			else if(strcasecmp(sxml_get_element_name(np), "login") == 0)
			{
				memcpy(recs[pos].login, content, sizeof(recs[pos].login));
			}
			else if(strcasecmp(sxml_get_element_name(np), "logout") == 0)
			{
				memcpy(recs[pos].logout, content, sizeof(recs[pos].logout));
			}
			else if(strcasecmp(sxml_get_element_name(np), "update") == 0)
			{
				memcpy(recs[pos].update, content, sizeof(recs[pos].update));
			}
			else if(strcasecmp(sxml_get_element_name(np), "delete") == 0)
			{
				memcpy(recs[pos].del, content, sizeof(recs[pos].del));
			}
		}
	}
}

// private 解析客户端列表
void parse_site_info(sxml_node_t *root)
{
	const char * content;	
	sxml_node_t *node, *np;

	node = sxml_find_element(root, "site", NULL, NULL);
	if(node != NULL)
	{
		for (np = sxml_get_child(node); np != NULL; np = sxml_get_next_sibling(np))
		{
			if (sxml_get_type(np) != SXML_ELEMENT) { continue; }
			if (sxml_get_element_name(np) == NULL) { continue; }
			
			if(conf.used == 0)
			{
				conf.site = (struct loong_site *)realloc(conf.site, sizeof(struct loong_site) * conf.num * SITE_CHUNK);
				conf.used = SITE_CHUNK;

				if(conf.site == NULL)
				{
					printf("realloc: %s\r\n", strerror(errno));
					exit(0);
				}
			}

			content = sxml_get_content(sxml_get_child(np));
			if(strcasecmp(sxml_get_element_name(np), "list") == 0)
			{
				parse_site_list(np);
				
				conf.num++;
				conf.used--;
			}
		}
	}
}

// private 解析数据库配置信息
void parse_mysql_info(sxml_node_t *root)
{
	const char * content;	
	sxml_node_t *node, *np;

	node = sxml_find_element(root, "mysql", NULL, NULL);
	if(node != NULL)
	{
		for (np = sxml_get_child(node); np != NULL; np = sxml_get_next_sibling(np))
		{
			if (sxml_get_type(np) != SXML_ELEMENT) { continue; }
			if (sxml_get_element_name(np) == NULL) { continue; }

			content = sxml_get_content(sxml_get_child(np));
			if(strcasecmp(sxml_get_element_name(np), "host") == 0)
			{
				memcpy(conf.host, content, sizeof(conf.host));
			}
			else if(strcasecmp(sxml_get_element_name(np), "port") == 0)
			{
				conf.port = atoi(content);
			}
			else if(strcasecmp(sxml_get_element_name(np), "user") == 0)
			{
				memcpy(conf.user, content, sizeof(conf.user));
			}
			else if(strcasecmp(sxml_get_element_name(np), "pass") == 0)
			{
				memcpy(conf.pass, content, sizeof(conf.pass));
			}
			else if(strcasecmp(sxml_get_element_name(np), "dbname") == 0)
			{
				memcpy(conf.dbname, content, sizeof(conf.dbname));
			}
		}
	}
}

// private 初始化解析配置文件
void parse_conf_init()
{
	memset(&conf, 0, sizeof(conf));
	
	conf.site = NULL;
	conf.num  = 0;
	conf.site = (struct loong_site *)calloc(SITE_CHUNK, sizeof(struct loong_site));
	conf.used = SITE_CHUNK;

	if(conf.site == NULL)
	{
		printf("realloc: %s\r\n", strerror(errno));
		exit(0);
	}
}

// public 注销解析文件的内存
void parse_conf_destroy()
{
	if(conf.site != NULL)
	{
		free(conf.site);
		conf.site = NULL;
	}
}

// public 开始解析配置文件
int parse_conf(char *filename)
{
	int fd;
	sxml_node_t *root;
	
	if ((fd = open(filename, O_RDONLY, 0400)) != -1)
	{
		parse_conf_init();

		if ((root = sxml_parse_file(fd)) != NULL) 
		{
			parse_root_info(root);
			parse_site_info(root);
			parse_mysql_info(root);

			sxml_delete_node(root);
		}
		close(fd);
		return 1;
	}

	printf("open(%s): %s\r\n", filename, strerror(errno));
	exit(0);
	return 0;
}

/*
int main(int argc, char *argv[])
{
	int i;
	struct loong_site *recs;
	
	parse_conf("test.xml");
	recs = conf.site;
	printf("server_port = %d\r\n", conf.server_port);

	printf("host = %s\tuser = %s\tpass = %s\tdbname = %s\tport = %d\ttable = %d\r\n", conf.host, conf.user, conf.pass, conf.dbname, conf.port, conf.chunk);
	
	for(i=0; i<conf.num; i++)
	{
		printf("id = %s\tkey = %s\tlogin = %s\t logout = %s\r\n", recs[i].id, recs[i].key, recs[i].login, recs[i].logout);
	}
	
	printf("conf.num = %d\tconf.used = %d\r\n", conf.num, conf.used);

	parse_conf_destroy();

	return 0;
}
*/

