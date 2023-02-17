#pragma once
#include "abi-bits/termios.h"
#include "libk/ringbuffer.h"
#include <abi-bits/termios.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <posix/termios.h>
#include <proc/proc.h>

#define TTY_BUFSIZE 0x1000
#define TTY_MAJOR 5
#define MAX_TTYS 8

struct tty;

struct tty_ldisc {
  int magic;
  char name[256];
  int flags;
  int i_ldiscs; // index to the global array of ldiscs

  int (*open)();
  int (*close)();

  size_t (*read)(struct tty *, size_t, uint8_t *);
  size_t (*write)(struct tty *, size_t, uint8_t *);

  int (*ioctl)(struct tty *, uint64_t request, void *arg);
  int (*poll)(struct pollfd fd, int timeout);

  int (*set_termios)(struct tty *, struct termios);
  void (*flush_buffer)(struct tty *);

  size_t (*receive_room)(struct tty *, size_t, char *);
  void (*receive_buf)(struct tty *, size_t, char *);
  void (*write_wakeup)(struct tty *);
};

struct tty_driver {
  struct tty *tty_table;

  char driver_name[256];
  char name[256];

  int dev_major;
  int num_devices;

  size_t (*read)(struct tty *, size_t, uint8_t *);
  size_t (*write)(struct tty *, size_t, uint8_t *);
  void (*put_char)(struct tty *, char c);
  void (*flush_chars)(struct tty *);
  size_t (*write_room)(struct tty *);

  int (*set_termios)(struct tty *, struct termios);
  int (*set_ldisc)(struct tty *, struct tty_ldisc);

  int (*ioctl)(struct tty *, uint64_t request, void *arg);
  int (*poll)(struct tty *, struct pollfd *fd, int timeout);
};

struct file_node {
  struct file *filep;
  struct file_node *next;
};

typedef struct file_node file_list_t;

struct tty {

  struct tty_driver driver;
  void *driver_data;

  void *private_data;

  RingBuffer *ibuf;
  RingBuffer *obuf;

  struct tty_ldisc ldisc;
  struct termios tios;
  struct winsize wsize;

  file_list_t tty_files;

  void (*flush_to_ldisc)(struct tty *);
};

extern VNodeOps tty_vnops;

void tty_register_driver(struct tty_driver driver);
void tty_default_put_char(struct tty *, char c);

int tty_register(dev_t dev, struct tty *tty, char *);

int tty_init_node(VFSNode *tty_node);

int tty_init();
int pty_init();
