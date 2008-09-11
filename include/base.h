#ifndef	_BASE_H
#define	_BASE_H

#include <tcadb.h>
#include <mysql.h>
#include <stdint.h>
#include <tcutil.h>
#include <stdbool.h>
#include "_config.h"

#define DATA_BUFFER_SIZE   1024
#define MAX_DEFAULT_FDS    10000


typedef enum 
{
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_HEAD,
	HTTP_METHOD_UNKNOWN
} http_method;


typedef struct
{
	int          sfd;
	time_t       now;
	uint64_t     code;
	TCMAP        *recs;
	uint32_t     events;
	http_method  method;
	char         ip[16];
}loong_conn;

struct loong_passwd
{
	uint64_t      id;
	char          pass[34];
	unsigned int  loong_status;
};

typedef enum 
{
	HTTP_RESPONSE_TIMEOUT,
	HTTP_RESPONSE_SIGN_NO,
	HTTP_RESPONSE_EMAIL_NO,
	HTTP_RESPONSE_EMAIL_OK,
	HTTP_RESPONSE_LOGIN_OK,
	HTTP_RESPONSE_DB_ERROR,
	HTTP_RESPONSE_LOGOUT_OK,
	HTTP_RESPONSE_DELETE_OK,
	HTTP_RESPONSE_DELETE_NO,
	HTTP_RESPONSE_UPDATE_OK,
	HTTP_RESPONSE_REGISTER_OK,
	HTTP_RESPONSE_VALIDATE_NO,
	HTTP_RESPONSE_VALIDATE_OK,
	HTTP_RESPONSE_USERNAME_NO,
	HTTP_RESPONSE_USERNAME_OK,
	HTTP_RESPONSE_PASSWORD_NO,
	HTTP_RESPONSE_CACHE_ERROR,
	HTTP_RESPONSE_EMAIL_EXISTS,
	HTTP_RESPONSE_UNKNOWN_TYPE,
	HTTP_RESPONSE_VARIABLE_ERROR,
	HTTP_RESPONSE_UNKNOWN_MODULE,
	HTTP_RESPONSE_USERNAME_EXISTS,
	HTTP_RESPONSE_SITE_NOT_EXISTS,
	HTTP_RESPONSE_EMAIL_NOT_EXISTS,
	HTTP_RESPONSE_USERNAME_NOT_EXISTS
} http_response_t;


typedef enum 
{
	LOONG_RESPONSE_LOGIN,
	LOONG_RESPONSE_UPDATE,
	LOONG_RESPONSE_DELETE,
	LOONG_RESPONSE_REGISTER
} loong_response_status;

struct loong_cmd_list
{
	char *cmd_name;
	int (*cmd_handler)(loong_conn *conn);
};



#define TABLE_STRUCTURE   \
	"CREATE TABLE IF NOT EXISTS `member_%d` ("                          \
	  "`uid` bigint(20) unsigned NOT NULL DEFAULT '0' COMMENT 'ID',"    \
	  "`username` varchar(20) NOT NULL,"                                \
	  "`password` varchar(33) NOT NULL DEFAULT '',"                     \
	  "`email` varchar(30) NOT NULL,"                                   \
	  "`ip` int(11) unsigned NOT NULL DEFAULT '0',"                     \
	  "`sex` tinyint(1) unsigned NOT NULL DEFAULT '0',"                 \
	  "`reg_time` int(11) unsigned NOT NULL DEFAULT '0',"               \
	  "`c_status` tinyint(1) unsigned NOT NULL DEFAULT '1',"            \
	  "PRIMARY KEY (`uid`),"                                            \
	  "KEY `username` (`username`)"                                     \
	") ENGINE=MyISAM"



MYSQL *dbh;

TCHDB *loong_user;

TCHDB *loong_mail;

TCHDB *loong_info;


#endif

