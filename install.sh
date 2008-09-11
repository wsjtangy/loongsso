#!/bin/bash

gcc -c md5.c -I.. -I../include -O3 -fno-guess-branch-probability
gcc -c sxml.c -I.. -I../include -O3 -fno-guess-branch-probability
gcc -c estring.c -I.. -I../include -O3 -fno-guess-branch-probability
gcc -c parse_conf.c -I.. -I../include -I/usr/local/tokyocabinet/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
gcc -c module.c -I.. -I../include -I/usr/local/tokyocabinet/include -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
gcc -c hash.c -I.. -I../include -I/usr/local/tokyocabinet/include -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
gcc -c util.c -I.. -I../include -I/usr/local/tokyocabinet/include -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
gcc -c protocol.c -I.. -I../include -I/usr/local/tokyocabinet/include -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
gcc -c evio_epoll.c -I.. -I../include -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
gcc -o loong loong.c -I.. -I../include -I/usr/local/tokyocabinet/include -I/usr/local/mysql/include/mysql -L/usr/local/tokyocabinet/lib -L/usr/local/gd2/lib -L/usr/local/mysql/lib/mysql -lmysqlclient -lgd -ltokyocabinet hash.o protocol.o module.o util.o md5.o sxml.o parse_conf.o estring.o evio_epoll.o -lrt -lz -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME