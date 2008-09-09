#ifndef	_PARSE_CONF_H
#define	_PARSE_CONF_H

#define SITE_CHUNK   10


struct loong_site
{
	char   id[30];
	char   key[30];
	char   update_key[30];

//------要回调的url参数
	char   del[128];
	char   login[128];
	char   logout[128];
	char   update[128];
};	

struct loong_config
{
	u_short server_id;
	u_short server_port;

	//mysql 相关参数
	size_t port;       //mysql的端口
	size_t chunk;      //分表的个数
	char   host[30];   //mysql的地址
	char   user[30];   //mysql的用户名称
	char   pass[30];   //mysql的密码
	char   dbname[30]; //mysql的数据库名称
	char   domain[30]; //loong SSO domain

	//SSO 应用的客户端

	size_t num;                 //客户端的个数
	size_t used;                //用了多少
	struct loong_site *site;
};	

struct loong_config conf;


// public 开始解析配置文件
int parse_conf(char *filename);

// public 注销解析文件的内存
void parse_conf_destroy();
#endif

