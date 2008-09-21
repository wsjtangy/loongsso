#!/bin/bash

#gcc -c estring.c evio_kqueue.c md5.c parse_conf.c sxml.c util.c protocol.c module.c loong_log.c hash.c -I../ -I../include/ -I/usr/local/mysql/include/mysql/ -I/home/lijinxing/soft/tokyocabinet-1.3.9/ -I/usr/local/gd2/include/ -DCHINA_USERNAME -DHAVE_SYS_EVENT_H -O2 -fno-guess-branch-probability
#gcc -o loong loong.c -I/usr/home/lijinxing/soft/tokyocabinet-1.3.9 -I./ -I/usr/local/mysql/include/mysql -L/usr/local/gd2/lib -L/usr/local/mysql/lib/mysql /usr/home/lijinxing/soft/tokyocabinet-1.3.9/libtokyocabinet.a -lmysqlclient -lgd hash.o protocol.o module.o util.o md5.o sxml.o parse_conf.o estring.o -lrt -lz -O3 -fno-guess-branch-probability evio_kqueue.o -DHAVE_SYS_EVENT_H -DCHINA_USERNAME -I../include -I../
#./configure --prefix=/home/lijinxing/loong --gd=/usr/local/gd2 --mysql=/usr/local/mysql --cache=/usr/local/tokyocabinet --user=2 --table=10


gcc -c md5.c -O3 -fno-guess-branch-probability
gcc -c sxml.c -O3 -fno-guess-branch-probability
gcc -c estring.c -O3 -fno-guess-branch-probability
gcc -c parse_conf.c -O3 -fno-guess-branch-probability
gcc -c evio_kqueue.c -O3 -fno-guess-branch-probability
gcc -c module.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -c hash.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -c util.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -c protocol.c -I./ -I/usr/home/lijinxing/soft/tokyocabinet-1.3.8 -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql -O3 -fno-guess-branch-probability
gcc -o loong loong.c -I/usr/home/lijinxing/soft/tokyocabinet-1.3.9 -I./ -I/usr/local/mysql/include/mysql -L/usr/local/gd2/lib -L/usr/local/mysql/lib/mysql /usr/home/lijinxing/soft/tokyocabinet-1.3.9/libtokyocabinet.a -lmysqlclient -lgd hash.o protocol.o module.o util.o md5.o sxml.o parse_conf.o estring.o -lrt -lz -O3 -fno-guess-branch-probability evio_kqueue.o -DHAVE_SYS_EVENT_H