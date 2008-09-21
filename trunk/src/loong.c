#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <loong.h>


//gcc -o loong loong.c -I/usr/local/qdbm/include/ -I./ -L/usr/local/qdbm/lib/ -L/usr/local/gd2/lib -lgd -lqdbm hash.o protocol.o code.o -lrt

struct loong_cmd_list loong_cmds[] = {
        {"login",    loong_sso_login    },
		{"check",    loong_sso_check    },
		{"update",   loong_sso_update   },
		{"delete",   loong_sso_delete   },
		{"logout",   loong_sso_logout   },
		{"validate", loong_sso_validate },
        {"register", loong_sso_register },
        { NULL,      NULL               }
};


void loong_conn_exit(loong_conn *conn)
{
	em.del(&ct, conn->sfd); 
	close(conn->sfd); 
	tcmapdel(conn->recs);
	free(conn);
}

void loong_client(void *arg)
{
	int n, i; 
	const char *module;
	
	loong_conn *conn = (loong_conn *)arg;

	n = loong_read(conn);
	if(n == -1)
	{
		send_response(conn, HTTP_RESPONSE_UNKNOWN_MODULE, NULL);
		return ;
	}

	module = tcmapget2(conn->recs, "module");
	if(module == NULL)
	{
		send_response(conn, HTTP_RESPONSE_UNKNOWN_MODULE, NULL);
		return ;
	}

	for (i = 0; loong_cmds[i].cmd_name; i++) 
	{
		if (strcmp(module, loong_cmds[i].cmd_name) == 0) 
		{
			loong_cmds[i].cmd_handler(conn);
			return ;
		}
	}

	send_response(conn, HTTP_RESPONSE_UNKNOWN_MODULE, NULL);	
}

void loong_accept(int sockfd)
{

	loong_conn *conn;
	int clilen, cfd;
	struct sockaddr_in clientaddr;
	
	clilen = sizeof(clientaddr);
	
	while ((cfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clilen)) < 0)
	{
		switch (errno)
		{
			case EAGAIN:
			case EINTR:
			case ECONNABORTED:
				continue;
			default:
				printf("accept returned %s", strerror(errno));
		}
	}

	set_nonblocking(cfd);

	conn          = (loong_conn *)malloc(sizeof(loong_conn));
	conn->sfd     = cfd;
	conn->code    = 0;
	conn->recs    = tcmapnew();
	conn->now     = time((time_t*)0);
	conn->method  = HTTP_METHOD_UNKNOWN;
	
	memset(conn->ip, 0, sizeof(conn->ip));
	memcpy(conn->ip, inet_ntoa(clientaddr.sin_addr), sizeof(conn->ip));

#ifdef HAVE_SYS_EVENT_H
	em.add(&ct, conn->sfd, EVIO_IN, (void *)conn);
#elif HAVE_SYS_EPOLL_H
	em.add(&ct, conn->sfd, EVIO_IN|EVIO_ERR|EVIO_ET, (void *)conn);
#endif
	
}


int loong_read(loong_conn *conn)
{
	int n, len;
	fd_set readfds;
	struct timeval tv;
	char buf[DATA_BUFFER_SIZE];
	
	len = 0;
	
	tv.tv_sec  = 10;
	tv.tv_usec = 0;
	
	FD_ZERO(&readfds);
	FD_SET(conn->sfd, &readfds);
	
	select(conn->sfd+1, &readfds, NULL, NULL, &tv);
	
	if (!FD_ISSET(conn->sfd, &readfds)) 
	{
		// timeout
		return -1; 
	}

	memset(buf, 0, sizeof(buf));
	while ((n = recv(conn->sfd, buf, DATA_BUFFER_SIZE, 0)) < 0) 
	{
		if (errno == EINTR)
			continue;
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			return -1;
	}

	parse_http_header(buf, strlen(buf), conn);

	return n;
}


int loong_write(loong_conn *conn, char const *buf, int nbyte)
{
	int n;

	while ((n = send(conn->sfd, buf, nbyte, 0)) < 0) 
	{
		if (errno == EINTR)
			continue;
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			return -1;
	}
	return n;
}

static void sig_listen(int sig)
{
	em.free(&ct);
	close(sock_fd);

	mysql_close(dbh);
	tchdbclose(loong_info);
	tchdbclose(loong_mail);
	tchdbclose(loong_user);

	tchdbdel(loong_info);
	tchdbdel(loong_mail);
	tchdbdel(loong_user);
	
	hash_destroy(codepool);
	parse_conf_destroy();
	exit(1);
}


int main(int argc, char **argv)
{
    int cfd;
    char sql[512];
	int sockfd;
    int n, i, rc, sql_len;
	my_bool reconnect = 0;
	

	//解析配置信息
	parse_conf(LOONG_CONFIG_PATH);
	
	codepool = hash_new(1000);
	dbh      = mysql_init (NULL);
    if (dbh == NULL)
    {
        printf ("mysql_init() failed (probably out of memory)\r\n");
        return 0;
    }
	//设置MySQL自动重连
	mysql_options(dbh, MYSQL_OPT_RECONNECT, &reconnect);


	//CLIENT_MULTI_STATEMENTS
	if (mysql_real_connect (dbh, conf.host, conf.user, conf.pass, conf.dbname, conf.port, NULL, CLIENT_MULTI_STATEMENTS) == NULL)
    {
        printf ("mysql_real_connect() failed\r\n");
        return 0;
    }
	
	for(i=0; i<TABLE_CHUNK; i++)
	{
		memset(sql, 0, sizeof(sql));
		sql_len = snprintf(sql, sizeof(sql), TABLE_STRUCTURE, i);

		rc = mysql_real_query(dbh, sql, sql_len);
		if(rc)
		{
			printf("Error making query: %s\n", mysql_error(dbh));
			sig_listen(0);
			exit(0);
		}
	}

	loong_info = tchdbnew();
	if(!tchdbopen(loong_info, LOONG_INFO_PATH, HDBOWRITER | HDBOCREAT))
	{
		rc = tchdbecode(loong_info);
		printf("loong_info.db open error: %s\r\n", tchdberrmsg(rc));
		sig_listen(0);
		return 0;
	}

	loong_mail = tchdbnew();
	if(!tchdbopen(loong_mail, LOONG_MAIL_PATH, HDBOWRITER | HDBOCREAT))
	{
		rc = tchdbecode(loong_info);
		printf("loong_mail.db open error: %s\r\n", tchdberrmsg(rc));
		sig_listen(0);
		return 0;
	}

	loong_user = tchdbnew();
	if(!tchdbopen(loong_user, LOONG_USER_PATH, HDBOWRITER | HDBOCREAT))
	{
		rc = tchdbecode(loong_info);
		printf("loong_user.db open error: %s\r\n", tchdberrmsg(rc));
		sig_listen(0);
		return 0;
	}
	
	daemon(1, 1);

	sockfd = make_socket();
 
	set_nonblocking(sockfd);

    em.init(&ct, MAX_DEFAULT_FDS);
#ifdef HAVE_SYS_EVENT_H
	em.add(&ct, sockfd, EVIO_IN, (void *)&sockfd);
#elif HAVE_SYS_EPOLL_H
	em.add(&ct, sockfd, EVIO_IN|EVIO_ET, (void *)&sockfd);
#endif

	signal(SIGTERM, sig_listen);
	signal(SIGINT,  sig_listen);
	signal(SIGQUIT, sig_listen);
	signal(SIGSEGV, sig_listen);
	signal(SIGALRM, sig_listen);
	signal(SIGPIPE, sig_listen);

	em.wait(&ct, -1, sockfd, loong_accept, loong_client);

	return 0;
}

