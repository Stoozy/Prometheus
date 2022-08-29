#pragma once
#include <fs/vfs.h>
#include <proc/proc.h>

#define TTY_BUF_SIZE 0x1000

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

  uint64_t outdev;
  uint64_t indev;

} Tty;


uint64_t tty_dev_write(Tty* tty, size_t size, unsigned char *buffer);
