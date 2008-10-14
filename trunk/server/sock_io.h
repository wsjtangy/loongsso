#ifndef SOCK_IO_H
#define SOCK_IO_H

void sock_init();

void fd_open(int fd);

void sock_close(int fd);

static void fd_close(int fd);

static int sock_listen_open();

int sock_set_noblocking(int fd);

static void sock_set_cork(int fd);

static void sock_set_linger(int fd);

static void sock_set_reuseaddr(int fd);

int sock_set_options(int fd);

#endif
