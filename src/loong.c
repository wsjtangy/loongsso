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
	int clilen, cfd, flags = 1;
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

	if (ioctl(cfd, FIONBIO, &flags) && ((flags = fcntl(cfd, F_GETFL, 0)) < 0 || fcntl(cfd, F_SETFL, flags | O_NONBLOCK) < 0)) 
	{
		close(cfd);
		return ;
	}

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


int make_socket() 
{
	struct sockaddr_in local_sa;
	
	int sock, reuse_addr = 1;
	
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
	
	local_sa.sin_family      = AF_INET;
	local_sa.sin_port        = htons(conf.server_port);
	local_sa.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if (bind(sock, (struct sockaddr*)&local_sa, sizeof(struct sockaddr_in)) < 0) 
	{
		perror("Binding socket.");
		exit(EXIT_FAILURE);
	}
	
	return sock;
}


int main(int argc, char **argv)
{
    int cfd;
    char sql[512];
	int flags = 1;
	int reuse_addr = 1;
    int n, i, rc, sql_len;
	struct eph_comm *conn;
    struct sockaddr_in addr;
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

    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);    
    if (sock_fd == -1)
    {
        perror("socket error :");
		sig_listen(0);
        return 1;
    }
    
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(conf.server_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind (sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
		perror("bind error :");
		sig_listen(0);
        return 1;
    }

    listen(sock_fd, 8192);

	if (ioctl(sock_fd, FIONBIO, &flags) && ((flags = fcntl(sock_fd, F_GETFL, 0)) < 0 || fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0)) 
	{
		sig_listen(0);
		return -1;
	}

    em.init(&ct, MAX_DEFAULT_FDS);
#ifdef HAVE_SYS_EVENT_H
	em.add(&ct, sock_fd, EVIO_IN, (void *)&sock_fd);
#elif HAVE_SYS_EPOLL_H
	em.add(&ct, sock_fd, EVIO_IN|EVIO_ET, (void *)&sock_fd);
#endif

	signal(SIGTERM, sig_listen);
	signal(SIGINT,  sig_listen);
	signal(SIGQUIT, sig_listen);
	signal(SIGSEGV, sig_listen);
	signal(SIGALRM, sig_listen);
	signal(SIGPIPE, sig_listen);

	daemon(1, 1);
	em.wait(&ct, -1, sock_fd, loong_accept, loong_client);

	return 0;
}

