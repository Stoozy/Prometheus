#include "kmalloc.h"
#include "libc/abi-bits/fcntl.h"
#include <fs/vfs.h>
#include <kprintf.h>
#include <string/string.h>
#include <util.h>

#include <libc/asm/ioctls.h>

#define TTY_BUF_SIZE 0x1000
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

struct tty {
  int id;

  File *file;
  char *buffer;
  size_t buffer_size;

  int lock;
};

static struct tty g_ttys[MAX_TTYS];
static int g_tty_counter = 0;

static struct tty *tty_find(const char *name) {
  for (int i = 0; i < MAX_TTYS; i++) {
    if (strcmp(g_ttys[i].file->name, name) == 0) {
      return &g_ttys[MAX_TTYS];
    }
  }
  return NULL;
}

struct file *tty_open(const char *filename, int flags) {
  kprintf("[TTY]  opening new tty file\n");
  struct tty *found = tty_find(filename);

  if (found)
    return found->file;

  // create tty
  if (g_tty_counter >= MAX_TTYS) {
    kprintf("Too many tty pairs\n");
    for (;;)
      ;
  } else {
    struct tty new_tty = (struct tty){.id = g_tty_counter,
                                      .buffer = kmalloc(TTY_BUF_SIZE),
                                      .buffer_size = TTY_BUF_SIZE,
                                      .lock = 0};

    File *tty_file = kmalloc(sizeof(File));
    tty_file->name = kmalloc(strlen(filename) + 1);
    strcpy(tty_file->name, filename);
    *(tty_file->name + strlen(filename)) = '\0'; // null terminate
    tty_file->size = TTY_BUF_SIZE;
    tty_file->position = 0;
    tty_file->fs = ttydev;
    tty_file->inode = g_tty_counter;
    tty_file->device = MKDEV(5, 0);

    g_ttys[g_tty_counter] = new_tty;
    g_tty_counter++;

    kprintf("[TTY]  Created %s with bufsize %d\n", tty_file->name,
            tty_file->size);
    return tty_file;
  };

  return NULL;
}

uint64_t tty_write(struct file *file, size_t size, u8 *buffer) {

  kprintf("[TTY]  Writing to %s\n STUB", file->name);
  return -1;
}

uint64_t tty_read(struct file *file, size_t size, u8 *buffer) {
  kprintf("[TTY]  Reading from %s STUB\n", file->name);
  return -1;
}

int tty_poll(struct file *file, struct pollfd *fd) {
  kprintf("[TTY]  Polling %s    STUB\n", file->name);
  return -1;
}

void tty_close(struct file *f) {}

int tty_ioctl(struct file *file, uint32_t request, void *arg) {

  // TODO

  return 0;
}
