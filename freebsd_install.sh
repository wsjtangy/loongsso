#!/bin/bash

gcc -c md5.c -O3 -fno-guess-branch-probability
gcc -c sxml.c -O3 -fno-guess-branch-probability
gcc -c estring.c -O3 -fno-guess-branch-probability
gcc -c parse_conf.c -O3 -fno-guess-branch-probability
gcc -c evio_kqueue.c -O3 -fno-guess-branch-probability
gcc -c module.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -c hash.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -c util.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -c protocol.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -o loong loong.c -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I./ -I/usr/local/mysql/include/mysql -L/usr/local/gd2/lib -L/usr/local/mysql/lib/mysql /usr/home/lijinxing/soft/tokyocabinet-1.3.8/libtokyocabinet.a -lmysqlclient -lgd hash.o protocol.o module.o util.o md5.o sxml.o parse_conf.o estring.o -lrt -lz -O3 -fno-guess-branch-probability evio_kqueue.o -DHAVE_SYS_EVENT_H