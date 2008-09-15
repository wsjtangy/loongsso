CC= gcc
CFLAGS= -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
INCLUDES= -I.
LIBS= -L.

OBJS= bubble_sort.o insert_sort.o main.o select_sort.o utility.o shell_sort.o





# configureによってMakefile.inから徭�啜弔防�撹されました。
# Un*x Makefile for GNU tar program.
# Copyright (C) 1991 Free Software Foundation, Inc.

# このプログラムはフリ�`ソフトウェアです。GNU GPL(General
# Public License)の娼舞に児づくなら徭喇に壅塘下?俐屎して
# くださって�Y��です。...
...
...

SHELL = /bin/sh

#### システムコンフィギュレ�`ション何蛍の�_兵 ####

srcdir = .

# gccを聞う��栽はgccに原奉のfixincludesスクリプトを�g佩
# するか、gccに-traditionalオプションをつけて�g佩するか
# すべきです。そうしないとioctlコ�`ルが音屎�_にコンパイ
# ルされてしまうシステムがあります。
CC = gcc -O
YACC = bison -y
INSTALL = /usr/local/bin/install -c
INSTALLDATA = /usr/local/bin/install -c -m 644

# 協�xに紗えられるのは肝のものになります。
# -DSTDC_HEADERS        ANSI Cヘッダとライブラリがある��栽。
# -DPOSIX               POSIXヘッダとライブラリがある��栽。
# -DBSD42               sys/dir.h(-DPOSOXを聞わない並)と
#                       sys/file.hと`struct stat'のst_blocks
#                       がある��栽。
# -DUSG                 System V/ANSI C猟忖双とメモリ�v方?
#                       ヘッダ、sys/sysmacros.h 、fcntl.h 、
#                       getcwdがあり、vallocなしで、ndir.h
#                       (-DDIRENTを聞わない��栽)がある��栽。
# -DNO_MEMORY_H         USGかSTDC_HEADERSかでmemory.hをイン
#                       クル�`ドしない��栽。
# -DDIRENT              USGでndir.hの旗わりにdirent.hにする��
#                       栽。
# -DSIGTYPE=int         シグナルハンドラがvoidではなくintを卦
#                       す��栽。
# -DNO_MTIO             sys/mtio.h(magtape ioctls)がない��栽。
# -DNO_REMOTE           リモ�`トシェル、rexecがない��栽。
# -DUSE_REXEC           フォ�`クrsh、remshのかわりにリモ�`トテ
#                       �`プ�g佩のためにrexecを聞う。
# -DVPRINTF_MISSING     vprintf�v方がない(が_doprintはある)��栽。
# -DDOPRNT_MISSING      _doprnt�v方がない��栽。-DVPRINTF_MISSING
#                       も協�xする駅勣があります。
# -DFTIME_MISSING       ftimeシステムコ�`ルがない��栽。
# -DSTRSTR_MISSING      strstr�v方がない��栽。
# -DVALLOC_MISSING      valloc�v方がない��栽。
# -DMKDIR_MISSING       mkdirとrmdirシステムコ�`ルがない��栽。
# -DRENAME_MISSING      renameシステムコ�`ルがない��栽。
# -DFTRUNCATE_MISSING   ftruncateシステムコ�`ルがない��栽。
# -DV7                  Version 7 UNIX�h廠(�L豚���Yはして
#                       いません)。
# -DEMUL_OPEN3          哈方３つバ�`ジョンのopenがなくて、
#                       贋壓するシステムコ�`ルでこれをエミ
#                       ュレ�`トさせたい��栽。
# -DNO_OPEN3            哈方３つバ�`ジョンのopenがなくて、
#                       エミュレ�`ト井openの旗わりにtar -k
#                       オプションを�o�燭砲気擦燭���栽。
# -DXENIX               sys/inode.hがあり、これの94をイン
#                       クル�`ドする駅勣がある��栽。
#

DEFS =  -DSIGTYPE=int -DDIRENT -DSTRSTR_MISSING \
        -DVPRINTF_MISSING -DBSD42
# NO_REMOTEを協�xして腎っぽにしなかった��栽に、これを
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
# インスト�`ルするプログラムの光？につけるプリ
# フィックスで、宥械は腎っぽか`g'になります。
binprefix =

# tarをインスト�`ルするディレクトリ。
bindir = $(prefix)/bin

# Infoファイルをインスト�`ルするディレクトリ。
infodir = $(prefix)/info

#### システムコンフィギュレ�`ション何蛍の�K阻 ####

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
# getdate.yには(�筝�?�p富した)8つの��栽があります。
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
