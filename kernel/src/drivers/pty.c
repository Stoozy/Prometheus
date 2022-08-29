#include <drivers/tty.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>
#include <util.h>

#include <libc/abi-bits/fcntl.h>
#include <libc/asm/ioctls.h>

#define PTY_BUF_SIZE 0x1000
#define MAX_PTYS 8 // for pts/X (x is only one digit)

struct file *pty_open(const char *filename, int flags);
uint64_t pty_write(struct file *file, size_t size, u8 *buffer);
uint64_t pty_read(struct file *file, size_t size, u8 *buffer);
void pty_close(struct file *f);
int pty_ioctl(struct file *, uint32_t request, void *arg);
int pty_poll(struct file *, struct pollfd *fd);

FileSystem ptyfs = {.open = pty_open,
                    .read = pty_read,
                    .write = pty_write,
                    .close = pty_close,
                    .ioctl = pty_ioctl,
                    .poll = pty_poll};

struct fs *ptydev = &ptyfs;

static Tty g_ptys[MAX_PTYS];
static int g_pty_counter = 0;

static struct file *file_from_pty(Tty *pty) {
  File *file = kmalloc(sizeof(File));
  file->name = kmalloc(strlen("pts/X\0"));
  strcpy(file->name, "pts/X\0");
  file->name[4] = '0' + pty->id;

  file->inode = pty->id;
  file->size = TTY_BUF_SIZE;
  file->fs = ptydev;
  file->position = 0;
  file->device = MKDEV(5, 2);

  kprintf("[PTY]  Created %s with bufsize %d\n", file->name, file->size);

  return file;
}

struct file *pty_open(const char *filename, int flags) {
  kprintf("[PTY]  opening %s\n", filename);
  if (strcmp(filename, "ptmx") == 0) {
    // create new master/slave pair

    if (g_pty_counter >= MAX_PTYS) {
      kprintf("Too many pty pairs\n");
      for (;;)
        ;
    } else {

      Tty new_pty = (Tty){.id = g_pty_counter,
                          .buffer_in = NULL,
                          .buffer_out = NULL,
                          .in_size = PTY_BUF_SIZE,
                          .out_size = PTY_BUF_SIZE};
      new_pty.buffer_in = kmalloc(PTY_BUF_SIZE);
      new_pty.buffer_out = kmalloc(PTY_BUF_SIZE);
      new_pty.id = g_pty_counter;

      g_ptys[g_pty_counter] = new_pty;
      ++g_pty_counter;

      kprintf("[PTY]    Created master/slave pair\n");
      return file_from_pty(&new_pty);
    };
  } else if (starts_with(filename, "pts/")) {
    // check if it's trying to open `/dev/pts/X`
    // this works for now bc it only goes up to 8
    int id = filename[4] - '0';
    kprintf("[PTY]  pts id is %d\n", id);
    return file_from_pty(&g_ptys[id]);
  }

  return NULL;
}

uint64_t pty_write(struct file *file, size_t size, u8 *buffer) {
  kprintf("[PTY]  Writing to %s STUB\n", file->name);
  for (size_t i = 0; i < size; i++)
    kprintf("%c ", buffer[i]);

  Tty pty = g_ptys[file->inode];
  kprintf("Got tty buffer at %x\n", pty.buffer_out);
  memcpy(pty.buffer_out + file->position, buffer, size);
  pty.in = true;
  return size;
}

uint64_t pty_read(struct file *file, size_t size, u8 *buffer) {
  kprintf("[PTY]  Reading from tty%d STUB\n", file->inode);
  for (;;)
    ;
  Tty pty = g_ptys[file->inode];
  memcpy(buffer, pty.buffer_out + file->position, size);
  return size;
}

void pty_close(struct file *file) { return; }

int pty_poll(struct file *file, struct pollfd *fd) {
  Tty pty = g_ptys[file->inode];

  kprintf("[PTY]  Polling pts/%d  STUB\n", file->inode);
  int events = 0;
  switch (fd->events) {
  case POLLIN: {
    kprintf("POLLIN\n");
    if (pty.in) {
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

int pty_ioctl(struct file *file, uint32_t request, void *arg) {
  if (MAJOR(file->device) == 136) {
    kprintf("Calling ioctl on slave\n");
    for (;;)
      ;
  }
  switch (request) {
  case TIOCSPTLCK: {
    int *lock = (int *)arg;
    g_ptys[file->inode].inlock = *lock;
    break;
  }
  case TIOCGPTN: {
    int *ptn = (int *)arg;
    *ptn = MINOR(g_ptys[file->inode].indev);
    break;
  }
  default:
    kprintf("Unknown ioctl request %lu\n", request);
    for (;;)
      ;
  }

  return 0;
}
