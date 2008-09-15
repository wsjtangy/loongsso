CC= gcc
CFLAGS= -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
INCLUDES= -I.
LIBS= -L.

OBJS= bubble_sort.o insert_sort.o main.o select_sort.o utility.o shell_sort.o





# configureによってMakefile.inから自動的に生成されました。
# Un*x Makefile for GNU tar program.
# Copyright (C) 1991 Free Software Foundation, Inc.

# このプログラムはフリーソフトウェアです。GNU GPL(General
# Public License)の精神に基づくなら自由に再配布?修正して
# くださって結構です。...
...
...

SHELL = /bin/sh

#### システムコンフィギュレーション部分の開始 ####

srcdir = .

# gccを使う場合はgccに付属のfixincludesスクリプトを実行
# するか、gccに-traditionalオプションをつけて実行するか
# すべきです。そうしないとioctlコールが不正確にコンパイ
# ルされてしまうシステムがあります。
CC = gcc -O
YACC = bison -y
INSTALL = /usr/local/bin/install -c
INSTALLDATA = /usr/local/bin/install -c -m 644

# 定義に加えられるのは次のものになります。
# -DSTDC_HEADERS        ANSI Cヘッダとライブラリがある場合。
# -DPOSIX               POSIXヘッダとライブラリがある場合。
# -DBSD42               sys/dir.h(-DPOSOXを使わない事)と
#                       sys/file.hと`struct stat'のst_blocks
#                       がある場合。
# -DUSG                 System V/ANSI C文字列とメモリ関数?
#                       ヘッダ、sys/sysmacros.h 、fcntl.h 、
#                       getcwdがあり、vallocなしで、ndir.h
#                       (-DDIRENTを使わない場合)がある場合。
# -DNO_MEMORY_H         USGかSTDC_HEADERSかでmemory.hをイン
#                       クルードしない場合。
# -DDIRENT              USGでndir.hの代わりにdirent.hにする場
#                       合。
# -DSIGTYPE=int         シグナルハンドラがvoidではなくintを返
#                       す場合。
# -DNO_MTIO             sys/mtio.h(magtape ioctls)がない場合。
# -DNO_REMOTE           リモートシェル、rexecがない場合。
# -DUSE_REXEC           フォークrsh、remshのかわりにリモートテ
#                       ープ実行のためにrexecを使う。
# -DVPRINTF_MISSING     vprintf関数がない(が_doprintはある)場合。
# -DDOPRNT_MISSING      _doprnt関数がない場合。-DVPRINTF_MISSING
#                       も定義する必要があります。
# -DFTIME_MISSING       ftimeシステムコールがない場合。
# -DSTRSTR_MISSING      strstr関数がない場合。
# -DVALLOC_MISSING      valloc関数がない場合。
# -DMKDIR_MISSING       mkdirとrmdirシステムコールがない場合。
# -DRENAME_MISSING      renameシステムコールがない場合。
# -DFTRUNCATE_MISSING   ftruncateシステムコールがない場合。
# -DV7                  Version 7 UNIX環境(長期試験はして
#                       いません)。
# -DEMUL_OPEN3          引数３つバージョンのopenがなくて、
#                       存在するシステムコールでこれをエミ
#                       ュレートさせたい場合。
# -DNO_OPEN3            引数３つバージョンのopenがなくて、
#                       エミュレート版openの代わりにtar -k
#                       オプションを無効にさせたい場合。
# -DXENIX               sys/inode.hがあり、これの94をイン
#                       クルードする必要がある場合。
#

DEFS =  -DSIGTYPE=int -DDIRENT -DSTRSTR_MISSING \
        -DVPRINTF_MISSING -DBSD42
# NO_REMOTEを定義して空っぽにしなかった場合に、これを
# rtapelib.oにセットします。
RTAPELIB = rtapelib.o
LIBS =
DEF_AR_FILE = /dev/rmt8
DEFBLOCKING = 20

CDEBUG = -g
CFLAGS = $(CDEBUG) -I. -I$(srcdir) $(DEFS) \
        -DDEF_AR_FILE=\"$(DEF_AR_FILE)\" \
        -DDEFBLOCKING=$(DEFBLOCKING)
LDFLAGS = -g

prefix = /usr/local
# インストールするプログラムの各々につけるプリ
# フィックスで、通常は空っぽか`g'になります。
binprefix =

# tarをインストールするディレクトリ。
bindir = $(prefix)/bin

# Infoファイルをインストールするディレクトリ。
infodir = $(prefix)/info

#### システムコンフィギュレーション部分の終了 ####

SRC1 =  tar.c create.c extract.c buffer.c \
        getoldopt.c update.c gnu.c mangle.c
SRC2 =  version.c list.c names.c diffarch.c \
        port.c wildmat.c getopt.c
SRC3 =  getopt1.c regex.c getdate.y
SRCS =  $(SRC1) $(SRC2) $(SRC3)
OBJ1 =  tar.o create.o extract.o buffer.o \
        getoldopt.o update.o gnu.o mangle.o
OBJ2 =  version.o list.o names.o diffarch.o \
        port.o wildmat.o getopt.o
OBJ3 =  getopt1.o regex.o getdate.o $(RTAPELIB)
OBJS =  $(OBJ1) $(OBJ2) $(OBJ3)
AUX =   README COPYING ChangeLog Makefile.in  \
        makefile.pc configure configure.in \
        tar.texinfo tar.info* texinfo.tex \
        tar.h port.h open3.h getopt.h regex.h \
        rmt.h rmt.c rtapelib.c alloca.c \
        msd_dir.h msd_dir.c tcexparg.c \
        level-0 level-1 backup-specs testpad.c

all:    tar rmt tar.info

tar:    $(OBJS)
        $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

rmt:    rmt.c
        $(CC) $(CFLAGS) $(LDFLAGS) -o $@ rmt.c

tar.info: tar.texinfo
        makeinfo tar.texinfo

install: all
        $(INSTALL) tar $(bindir)/$(binprefix)tar
        -test ! -f rmt || $(INSTALL) rmt /etc/rmt
        $(INSTALLDATA) $(srcdir)/tar.info* $(infodir)

$(OBJS): tar.h port.h testpad.h
regex.o buffer.o tar.o: regex.h
# getdate.yには(変更?減少した)8つの競合があります。
# (getdate.y has 8 shift/reduce conflicts.)

testpad.h: testpad
        ./testpad

testpad: testpad.o
        $(CC) -o $@ testpad.o

TAGS:   $(SRCS)
        etags $(SRCS)

clean:
        rm -f *.o tar rmt testpad testpad.h core

distclean: clean
        rm -f TAGS Makefile config.status

realclean: distclean
        rm -f tar.info*

shar: $(SRCS) $(AUX)
        shar $(SRCS) $(AUX) | compress \
          > tar-`sed -e '/version_string/!d' \
                     -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
                     -e q
                     version.c`.shar.Z

dist: $(SRCS) $(AUX)
        echo tar-`sed \
             -e '/version_string/!d' \
             -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
             -e q
             version.c` > .fname
        -rm -rf `cat .fname`
        mkdir `cat .fname`
        ln $(SRCS) $(AUX) `cat .fname`
        -rm -rf `cat .fname` .fname
        tar chZf `cat .fname`.tar.Z `cat .fname`

tar.zoo: $(SRCS) $(AUX)
        -rm -rf tmp.dir
        -mkdir tmp.dir
        -rm tar.zoo
        for X in $(SRCS) $(AUX) ; do \
            echo $$X ; \
            sed 's/$$/^M/' $$X \
            > tmp.dir/$$X ; done
        cd tmp.dir ; zoo aM ../tar.zoo *
        -rm -rf tmp.dir
