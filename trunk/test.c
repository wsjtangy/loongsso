#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/event.h>
#include "evio_kqueue.h"

#define BUF_LEN  256  /* バッファのサイズ */
#define MAX_SOCK 256  /* 最大プロセス数 */

struct ev_ct ct;
                                /* クライアントの情報を保持する構造体 */
typedef struct CLIENT_INFO 
{
    char hostname[BUF_LEN];       /* ホスト名 */
    char ipaddr[BUF_LEN];         /* IP アドレス */
    int port;                     /* ポート番号 */
} CLIENT_INFO;

CLIENT_INFO client_info[MAX_SOCK+1];


typedef struct 
{
	int fd;
} loong_conn;

char msg[] = "HTTP/1.1 200 OK\r\nServer: qiye/RC1.0\r\nDate: Fri, 01 Aug 2008 12:29:20 GMT\r\nContent-Type: text/html\r\nContent-Length: 2\r\nLast-Modified: Tue, 04 Dec 2007 19:43:47 GMT\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\n\r\n12";



void accept_new_client(int sock)
{
    int len;
    int new_socket;
    loong_conn *conn;
	struct hostent *peer_host;
    struct sockaddr_in peer_sin;
	

    len = sizeof(peer_sin);
    new_socket = accept(sock, (struct sockaddr *)&peer_sin, &len);
    if ( new_socket == -1 )
	{
        perror("accept");
        exit(1);
    }
    
	conn = (loong_conn *)malloc(sizeof(loong_conn));

	conn->fd = new_socket;
	em.add(&ct, new_socket, EVIO_IN, (void *)conn);
}

/*-----------------------------------------------------
  引数でソケットディスクリプタを受け取り、そのソケットから
  read(2) で文字列を読み込み、文字列をそのままクライアントに
  送信する。
  -----------------------------------------------------*/
void
read_and_reply(void *arg)
{
    int read_size;
    int ret;
    char buf[BUF_LEN];
	
	loong_conn *conn = (loong_conn *)arg;

    read_size = read(conn->fd, buf, sizeof(buf)-1);

    if ( read_size == -1 ){
        perror("read");
    } else if ( read_size == 0 ){
        ret = close(conn->fd);
        if ( ret == -1 ){
            perror("close");
            exit(1);
        }
    }
	else 
	{
        write(conn->fd, msg, strlen(msg));
    }
	
	em.del(&ct, conn->fd);
	free(conn);
}

/*
void print_r(void *arg)
{
	int ret = (int)arg;

	printf("%d\r\n", *ret);
}
*/

int main()
{
    int sock_optval = 1;
    int ret;
    int port = 5000;
    int listening_socket;
    struct sockaddr_in sin;
	
	
    listening_socket = socket(AF_INET, SOCK_STREAM, 0);

                              
    if ( setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR,
                    &sock_optval, sizeof(sock_optval)) == -1 ){
        perror("setsockopt");
        exit(1);
    }
                               
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    ret = bind(listening_socket, (struct sockaddr *)&sin, sizeof(sin));
    if ( ret == -1 ){
        perror("bind");
        exit(1);
    }

    ret = listen(listening_socket, SOMAXCONN);
    if ( ret == -1 )
	{
        perror("listen");
        exit(1);
    }
    printf("ポート %d を見張ります。\n", port);
	
	em.init(&ct, 100);
	em.add(&ct, listening_socket, EVIO_IN, (void *)&listening_socket);
	em.wait(&ct, -1, listening_socket, accept_new_client, read_and_reply);

	/*
    int kq;
    struct kevent kev;

    kq = kqueue();
    if ( kq == -1 ){
        perror("kqueue");
        exit(1);
    }
    EV_SET(&kev, listening_socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
    ret = kevent(kq, &kev, 1, NULL, 0, NULL);
    if ( ret == -1 ){
        perror("kevent");
        exit(1);
    }


    while (1)
	{
        int n;
        struct timespec waitspec;    
        waitspec.tv_sec  = 2;         // 待ち時間に 2.500 秒を指定 
        waitspec.tv_nsec = 500000;

        n = kevent(kq, NULL, 0, &kev, 1, &waitspec);

        if ( n == -1 )
		{
            perror("kevent");
            exit(1);

        }
		else if ( n > 0 )
		{
                                // 読み込み可能なソケットが存在する 
            if ( kev.ident == listening_socket )
			{
                                // 新しいクライアントがやってきた 
                int new_sock = accept_new_client(kev.ident);
                if ( new_sock != -1 )
				{
                                // 監視対象に新たなソケットを追加 
                    EV_SET(&kev, new_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    n = kevent(kq, &kev, 1, NULL, 0, NULL);
                    if ( n == -1 ){
                        perror("kevent");
                        exit(1);
                    }
                }

            } else {
                int sock = kev.ident;
                printf("ディスクリプタ %d 番が読み込み可能です。\n", sock);
                read_and_reply(sock);
            }
        }
    }
    */
    close(listening_socket);
    return 0;
}

