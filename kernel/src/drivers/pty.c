#include "fs/devfs.h"
#include "fs/vfs.h"
#include <abi-bits/fcntl.h>
#include <asm/ioctls.h>
#include <drivers/tty.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/ringbuffer.h>

#include <string/string.h>

#include <abi-bits/linux/vt.h>
#include <posix/termios.h>

#define PTMX_MAJOR 5
#define PTMX_MINOR 2

#define MAX_LINE 1024
#define PTS_MAJOR 136

struct ptm_data;

RingBuffer *ptm_kbd_rb;

struct pts_data {
  int slave;
  struct tty *tty;

  struct ptm_data *master;
  struct winsize wsize;

  int lock;
};

struct ptm_data {
  RingBuffer ibuf;
  struct pts_data *slave;
};

int ptmx_slave_ctr = 0;

static File *ptmx_open(const char *filename, int flags);
static size_t ptm_read(struct file *, size_t, uint8_t *buf);
static size_t ptm_write(struct file *, size_t, uint8_t *buf);
static int ptm_poll(struct file *, struct pollfd *, int timeout);
static int ptm_ioctl(struct file *, uint32_t request, void *arg);

static void pts_flush_output(struct tty *);
static int pts_ioctl(struct tty *, uint64_t, void *);

FileSystem ptmx_ops = {.open = ptmx_open};
FileSystem ptm_ops = {
    .read = ptm_read, .write = ptm_write, .ioctl = ptm_ioctl, .poll = ptm_poll};

struct tty_driver pts_ops = {.flush_chars = pts_flush_output,
                             .ioctl = pts_ioctl};

int pty_init() {
  CharacterDevice *chardev = kmalloc(sizeof(CharacterDevice));

  chardev->fs = &ptmx_ops;
  chardev->dev = MKDEV(PTMX_MAJOR, PTMX_MINOR);
  chardev->name = kmalloc(256);
  strcpy(chardev->name, "ptmx");

  if (devfs_register_chardev(chardev) == -1)
    return -1;

  return 0;
}

static File *ptmx_open(const char *filename, int flags) {
  int slave_no = ptmx_slave_ctr++;

  File *file = kmalloc(sizeof(struct file));

  struct tty *pts_tty = tty_init_dev(pts_ops);

  if (!(flags & O_NOCTTY)) {
    extern struct tty *gp_active_tty;
    gp_active_tty = pts_tty;
  }

  struct pts_data *pts_data = kmalloc(sizeof(struct pts_data));
  struct ptm_data *ptm_data = kmalloc(sizeof(struct ptm_data));

  pts_tty->private_data = pts_data;
  pts_data->slave = slave_no;
  pts_data->tty = pts_tty;
  pts_data->master = ptm_data;

  rb_init(&ptm_data->ibuf, MAX_LINE, sizeof(char));
  ptm_kbd_rb = &ptm_data->ibuf;

  ptm_data->slave = pts_data;

  file->fs = &ptm_ops;
  file->private_data = ptm_data;
  file->name = kmalloc(256);
  sprintf(file->name, "/dev/pts/%d", slave_no);

  File *pts_file = kmalloc(sizeof(struct file));
  pts_file->name = file->name;
  extern FileSystem tty_fops;
  pts_file->private_data = pts_tty;
  pts_file->fs = &tty_fops;
  pts_file->device = MKDEV(PTS_MAJOR, slave_no);

  // tty_register(MKDEV(PTS_MAJOR, slave_no), pts_tty, pts_name);

  vfs_add_to_open_list(vfs_mknod(pts_file));

  kprintf("finshed creating pty pair\n");
  return file;
}

static size_t ptm_read(struct file *file, size_t size, uint8_t *buf) {
  kprintf("Reading %s\n", file->name);
  struct ptm_data *ptm = file->private_data;

  size_t s = 0;
read:
  disable_irq();

  for (; s < size; s++)
    if (!rb_pop(&ptm->ibuf, &buf[s]))
      break;
  enable_irq();
  /* extern ProcessControlBlock * running; */
  if (s == 0) {
    /* extern ProcessQueue kbd_wait_queue; */
    /* pqueue_push(&kbd_wait_queue, running); */
    /* block_process(running, WAITING); */
    goto read;
  }

  kprintf("Read %d bytes\n", s);
  // should be in line disc
  return s;
}

static size_t ptm_write(struct file *file, size_t size, uint8_t *buf) {
  struct ptm_data *ptm = file->private_data;
  struct pts_data *pts = ptm->slave;

  size_t w = 0;
  for (; w < size; w++) {
    if (!rb_push(pts->tty->ibuf, &buf[w]))
      break;
  }

  kprintf("Written %d bytes to %s\n", w, file->name);

  /* kprintf("PTY write: TODO\n"); */
  return w;
}

static int ptm_ioctl(struct file *filep, uint32_t request, void *arg) {
  struct ptm_data *ptm = filep->private_data;
  struct pts_data *pts = ptm->slave;

  switch (request) {
  case TIOCGPTN: {
    int *ptn = arg;
    *ptn = pts->slave;
    return 0;
  }
  case TCGETS: {
    struct termios *tios = arg;
    *tios = pts->tty->tios;
    return 0;
  }
  case TCSETS: {
    struct termios *tios = arg;
    pts->tty->tios = *tios;
    return 0;
  }
  case TIOCSPTLCK: {
    kprintf("Called TIOCSPTLCK");
    int *lock = arg;
    pts->lock = *lock;
    return 0;
  }
  case TIOCSPGRP: {
    // FIXME:
    return 0;
  }
  case TIOCGPGRP: {
    // FIXME: hardcoded value
    pid_t *grp = arg;
    *grp = 1000;
    return 0;
  }
  case TIOCGPTLCK: {
    kprintf("Called TIOCGPTLCK");
    int *lock = arg;
    *lock = pts->lock;
    return 0;
  }
  case TIOCGWINSZ: {

    struct winsize *ws = arg;
    *ws = pts->wsize;
    return 0;
  }
  case TIOCSWINSZ: {
    struct winsize *ws = arg;
    pts->wsize = *ws;
    return 0;
  }
  case TIOCSCTTY: {
    extern struct tty *gp_active_tty;
    gp_active_tty = pts->tty;
    return 0;
  }
  case VT_RELDISP:
  case VT_SETMODE:
    break;
  default: {
    kprintf("ioctl unimplemented 0x%x\n", request);
    for (;;)
      ;
  }
  }
  return 0;
}

static void pts_flush_output(struct tty *tty) {
  struct pts_data *pts = tty->private_data;
  struct ptm_data *ptm = pts->master;

  char ch;
  while (rb_pop(tty->obuf, &ch)) {
    if (!rb_push(&ptm->ibuf, &ch))
      break;
  }
}

static int pts_ioctl(struct tty *tty, uint64_t request, void *arg) {
  struct pts_data *pts_data = tty->private_data;

  switch (request) {
  case TIOCSCTTY: {
    extern struct tty *gp_active_tty;
    gp_active_tty = pts_data->tty;
    return 0;
  }
  case TIOCSWINSZ: {
    struct winsize *ws = arg;
    pts_data->wsize = *ws;
    return 0;
  }
  case TIOCGWINSZ: {
    struct winsize *ws = arg;
    *ws = pts_data->wsize;
    return 0;
  }
  case TCGETS: {
    struct termios *tios = arg;
    *tios = tty->tios;
    return 0;
  }
  case TCSETS: {
    struct termios *tios = arg;
    tty->tios = *tios;
    return 0;
  }
  case TIOCSPGRP: {
    pid_t *grp = arg;
    // FIXME:
    return 0;
  }
  case TIOCGPGRP: {
    pid_t *grp = arg;
    // FIXME: hardcoded value
    *grp = 200;
    return 0;
  }
  default: {
    kprintf("Unimplemented \n");
    for (;;)
      ;
  }
    return 0;
  }
}

static int ptm_poll(struct file *file, struct pollfd *pfd, int timeout) {
  /* struct pts_data *pts = ptm->slave; */
  /* struct tty *tty = pts->tty; */

  struct ptm_data *ptm = file->private_data;

  int evt = 0;
  if (pfd->events & POLLIN) {
    if (ptm->ibuf.len) {
      pfd->revents |= POLLIN;
      evt++;
    }
  }

  if (pfd->revents & POLLOUT) {
    pfd->revents |= POLLOUT;
    evt++;
  }

  return evt;
}
