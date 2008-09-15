CC= gcc
CFLAGS= -O3 -fno-guess-branch-probability -DHAVE_SYS_EPOLL_H -DCHINA_USERNAME
INCLUDES= -I.
LIBS= -L.

OBJS= bubble_sort.o insert_sort.o main.o select_sort.o utility.o shell_sort.o