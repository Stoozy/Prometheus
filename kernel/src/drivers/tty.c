#include "kmalloc.h"
#include "libc/abi-bits/fcntl.h"
#include <drivers/tty.h>
#include <fs/vfs.h>
#include <kprintf.h>
#include <stdint.h>
#include <string/string.h>
#include <util.h>

#include <libc/asm/ioctls.h>

#define MAX_TTYS 8

struct file *tty_open(const char *filename, int flags);
uint64_t tty_write(struct file *file, size_t size, u8 *buffer);
uint64_t tty_read(struct file *file, size_t size, u8 *buffer);
void tty_close(struct file *f);
int tty_ioctl(struct file *, uint32_t request, void *arg);
int tty_poll(struct file *, struct pollfd *fd);

FileSystem ttyfs = {.open = tty_open,
                    .read = tty_read,
                    .write = tty_write,
                    .close = tty_close,
                    .ioctl = tty_ioctl,
                    .poll = tty_poll};

struct fs *ttydev = &ttyfs;
Tty volatile *gp_active_tty = NULL;

static struct tty g_ttys[MAX_TTYS];

static int g_tty_counter = 0;

static Tty *tty_find(const char *name) {
  int id = name[3] - '0'; // ttyN
  if (g_ttys[id].buffer_in)
    return &g_ttys[id];

  return NULL;
}

static File *file_from_tty(Tty *tty) {
  File *file = kmalloc(sizeof(File));
  file->name = kmalloc(strlen("ttyX\0"));
  strcpy(file->name, "ttyX\0");
  file->name[3] = '0' + tty->id;

  file->inode = tty->id;
  file->size = TTY_BUF_SIZE;
  file->fs = ttydev;
  file->position = 0;
  file->device = MKDEV(5, 0);

  kprintf("[TTY]  Created %s with bufsize %d\n", file->name, file->size);
  return file;
}

struct file *tty_open(const char *filename, int flags) {
  kprintf("[TTY]  opening new tty file: %s\n", filename);
  Tty *found = tty_find(filename);

  if (found) {
    kprintf("Found tty%d\n", found->id);
    gp_active_tty = found;
    return file_from_tty(found);
  }

  // create tty
  if (g_tty_counter >= MAX_TTYS) {
    kprintf("Too many tty pairs\n");
    for (;;)
      ;
  }
  struct tty new_tty = (struct tty){.id = g_tty_counter,
                                    .buffer_in = NULL,
                                    .in_size = TTY_BUF_SIZE,
                                    .buffer_out = NULL,
                                    .out_size = TTY_BUF_SIZE,
                                    .inlock = 0,
                                    .outlock = 0};
  new_tty.buffer_out = kmalloc(TTY_BUF_SIZE);
  new_tty.buffer_in = kmalloc(TTY_BUF_SIZE);

  g_ttys[g_tty_counter] = new_tty;
  ++g_tty_counter;

  gp_active_tty = &new_tty;

  return file_from_tty(&new_tty);
}

uint64_t tty_dev_write(Tty *tty, size_t size, unsigned char *buffer) {
  kprintf("[TTY]  Writing to tty%d STUB\n", tty->id);
  kprintf("buffer is at %x\n", tty->buffer_in);
  for (size_t i = 0; i < size; i++)
    kprintf("%c ", buffer[i]);

  memcpy(tty->buffer_in, buffer, size);
  tty->in = true;

  return size;
}

uint64_t tty_write(struct file *file, size_t size, u8 *buffer) {
  kprintf("[TTY]  Writing to %s STUB\n", file->name);
  for (size_t i = 0; i < size; i++)
    kprintf("%c ", buffer[i]);

  Tty tty = g_ttys[file->inode];
  kprintf("Got tty buffer at %x\n", tty.buffer_out);
  memcpy(tty.buffer_out + file->position, buffer, size);
  // for (;;)
  //   ;
  return size;
}

void tty_flush(Tty *tty) { memset(tty->buffer_in, 0, TTY_BUF_SIZE); }

uint64_t tty_read(struct file *file, size_t size, u8 *buffer) {
  kprintf("[TTY]  Reading from tty%d STUB\n", file->inode);
  Tty tty = g_ttys[file->inode];
  memcpy(buffer, tty.buffer_in, size);

  tty_flush(&tty);
  tty.in = false;

  return size;
}

int tty_poll(struct file *file, struct pollfd *fd) {
  Tty tty = g_ttys[file->inode];

  kprintf("[TTY]  Polling tty%d  STUB\n", file->inode);
  int events = 0;
  switch (fd->events) {
  case POLLIN: {
    kprintf("POLLIN\n");
    if (tty.in) {
      fd->revents |= POLLIN;
      events++;
    }
    break;
  }
  case POLLOUT: {
    kprintf("POLLOUT\n");
    break;
  }
  default:
    break;
  }

  if (events)
    kprintf("Got event\n");

  return events;
}

void tty_close(struct file *f) {}

int tty_ioctl(struct file *file, uint32_t request, void *arg) {

  kprintf("Called ioctl on tty%d\n", file->inode);
  // TODO

  return 0;
}
