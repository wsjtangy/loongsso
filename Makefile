all: loong

# Which compiler
CC = gcc

# Where to install
INSTDIR = /home/lijinxing/bak

# Where are include files kept
INCLUDE = -I./ -I./include -I/usr/local/tokyocabinet/include -I/usr/local/gd2/include -I/usr/local/mysql/include/mysql
LIBS    = -L/usr/local/tokyocabinet/lib -L/usr/local/gd2/lib -L/usr/local/mysql/lib/mysql 
SRCS    = ./src/estring.c ./src/loong_log.c ./src/module.c ./src/protocol.c ./src/util.c ./src/evio_epoll.c  ./src/hash.c ./src/md5.c ./src/parse_conf.c  ./src/sxml.c
OBJS    = estring.o  md5.o  sxml.o loong_log.o module.o protocol.o util.o evio_epoll.o hash.o parse_conf.o
MYLIB   = -lmysqlclient -lgd -ltokyocabinet -lrt

# Options for development
CFLAGS = -O2 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME


loong:
	$(CC) -c $(SRCS) $(INCLUDE) $(CFLAGS); 
	$(CC) -o loong ./src/loong.c $(OBJS) $(INCLUDE) $(LIBS) $(MYLIB) $(CFLAGS); 
	@echo "\r\nPlease Run make install";


clean:
	-@rm $(OBJS) _config.h loong

install: 
	@if [ ! -d $(INSTDIR) ]; then \
	   mkdir $(INSTDIR); \
	fi
	
	cp loong $(INSTDIR); 
	cp -r ./conf/ $(INSTDIR);  
	cp -r ./font/ $(INSTDIR);  
	@echo "Installed in $(INSTDIR)"; 
	-@rm $(OBJS) _config.h loong;

