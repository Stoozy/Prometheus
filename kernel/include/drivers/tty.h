#pragma once
#include <fs/vfs.h>
#include <proc/proc.h>
#include <libc/termios.h>

#define TTY_BUF_SIZE 0x1000

struct ptm;

struct pts {
  int id;
  struct tty *tty;
  struct winsize ws;

  struct ptm *master;
};


struct ptm {
  int inlock;

  char *buffer;
  size_t bufsize;

  bool in;

  struct pts *slave;
};



typedef struct tty {
  int id;

  char * buffer_in;
  size_t in_size;

  char * buffer_out;
  size_t out_size;

  bool in;
  bool out;

  int inlock;
  int outlock;

} Tty;


