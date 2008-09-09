#ifndef __LOONG_H_
#define __LOONG_H_


#include <md5.h>
#include <base.h>
#include <hash.h>
#include <sxml.h>
#include <util.h>
#include <module.h>
#include <estring.h>
#include <protocol.h>
#include <parse_conf.h>

#ifdef HAVE_SYS_EVENT_H
#include <evio_kqueue.h>
#elif HAVE_SYS_EPOLL_H
#include <evio_epoll.h>
#endif

struct ev_ct ct;

hash  *codepool;

static int sock_fd;

void loong_accept(int sockfd);

void loong_client(void *arg);

void loong_conn_exit(loong_conn *conn);
#endif

