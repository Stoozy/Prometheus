#include "cpu/idt.h"
#include "fs/devfs.h"
#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include <abi-bits/fcntl.h>
#include <asm-generic/ioctls.h>
#include <asm-generic/poll.h>
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
  int slave_no;
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

static int ptmx_open(File *file, VFSNode *vn, int flags);
static int ptm_open(File *file, VFSNode *vn, int flags);

static ssize_t ptm_read(File *file, VFSNode *vn, void *buf, size_t nbyte,
                        off_t off);
static ssize_t ptm_write(File *file, VFSNode *vn, void *buf, size_t nbyte,
                         off_t off);
static int ptm_ioctl(VFSNode *vp, uint64_t request, void *arg, int fflag);
static int ptm_poll(VFSNode *vp, int events);

static void pts_flush_output(struct tty *tty);
static int pts_ioctl(struct tty *tty, uint64_t req, void *args);

VNodeOps ptmx_ops = {.open = ptmx_open};
VNodeOps ptm_ops = {.open = ptm_open,
                    .read = ptm_read,
                    .write = ptm_write,
                    .ioctl = ptm_ioctl,
                    .poll = ptm_poll};

struct tty_driver pts_tty_ops = {.flush_chars = pts_flush_output,
                                 .ioctl = pts_ioctl};

static int ptm_open(File *file, VFSNode *vn, int flags) {
  kprintf("ptm_open()\n");
  for (;;)
    ;
}
static int ptmx_open(File *file, VFSNode *vn, int flags) {
  kprintf("ptmx_open()\n");
  int slave_no = ptmx_slave_ctr++;

  VFSNode *pts_dirnode;
  if (dev_root->ops->lookup(dev_root, &pts_dirnode, "/dev/pts/")) {
    kprintf("Couldn't find /dev/pts/");
    return -1;
  }

  // return file with vnode as ptm node with ptm_ops
  // setup pts/N node with proper tty driver (pts_tty_ops)

  struct ptm_data *ptm_data = kmalloc(sizeof(struct ptm_data));
  struct pts_data *pts_data = kmalloc(sizeof(struct pts_data));

  /* Initialize ptm node (entirely virtual)*/
  VFSNode *ptm_node = kmalloc(sizeof(VFSNode));
  ptm_node->type = VFS_CHARDEVICE;
  ptm_node->ops = &ptm_ops;
  ptm_node->private_data = ptm_data;
  rb_init(&ptm_data->ibuf, MAX_LINE, sizeof(char));

  /* init pts node `/dev/pts/N` */
  VFSNode *pts_node;
  char name[256];
  sprintf(name, "/dev/pts/%d", slave_no);
  VAttr attr = {.type = VFS_CHARDEVICE, .rdev = MKDEV(PTMX_MAJOR, slave_no)};
  pts_dirnode->ops->create(pts_dirnode, &pts_node, name, &attr);

  tty_init_node(pts_node);
  TmpNode *pts_tnode = pts_node->private_data;
  struct tty *pts_tty = pts_tnode->dev.cdev.private_data;
  pts_tty->driver = pts_tty_ops;
  pts_tty->private_data = pts_data;

  file->vn = ptm_node;
  file->refcnt = 1;
  file->pos = 0;

  ptm_data->slave = pts_data;

  pts_data->tty = pts_tty;
  pts_data->master = ptm_data;

  return 0;
}

static ssize_t ptm_read(File *file, VFSNode *vn, void *buf, size_t nbyte,
                        off_t off) {
  kprintf("ptm_read()");
  struct ptm_data *ptm = vn->private_data;

  size_t s = 0;
read:
  disable_irq();

  for (; s < nbyte; s++)
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
static ssize_t ptm_write(File *file, VFSNode *vn, void *buf, size_t nbyte,
                         off_t off) {
  kprintf("ptm_write()");
  struct ptm_data *ptm = vn->private_data;
  struct pts_data *pts = ptm->slave;

  size_t w = 0;
  for (; w < nbyte; w++) {
    if (!rb_push(pts->tty->ibuf, &buf[w]))
      break;
  }
  return w;
}
static int ptm_ioctl(VFSNode *vp, uint64_t request, void *arg, int fflag) {
  kprintf("ptm_ioctl()\n");
  struct ptm_data *ptm = vp->private_data;
  struct pts_data *pts = ptm->slave;

  switch (request) {
  case TIOCGPTN: {
    int *ptn = arg;
    *ptn = pts->slave_no;
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
    kprintf("Unhandled VT_RELDISP and VT_SETMODE\n");
    for (;;)
      ;
    break;
  default: {
    kprintf("ioctl unimplemented 0x%x\n", request);
    for (;;)
      ;
  }
  }
  return 0;
}

static int ptm_poll(VFSNode *vp, int events) {
  // kprintf("ptm_poll()");
  int revents = 0;

  struct ptm_data *ptm = vp->private_data;
  struct pts_data *pts = ptm->slave;

  if (events & POLLIN) {
    if (ptm->ibuf.len) {
      revents |= POLLIN;
    }
  }

  if (events & POLLOUT) {
    revents |= POLLOUT;
  }

  return revents;
}

static void pts_flush_output(struct tty *tty) {
  kprintf("pts_flush_output()");
  struct pts_data *pts = tty->private_data;
  struct ptm_data *ptm = pts->master;

  char ch;
  while (rb_pop(tty->obuf, &ch)) {
    if (!rb_push(&ptm->ibuf, &ch))
      break;
  }
}
static int pts_ioctl(struct tty *tty, uint64_t req, void *arg) {
  kprintf("pts_ioctl()");
  struct pts_data *pts_data = tty->private_data;

  switch (req) {
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
    *grp = 1000;
    return 0;
  }
  default: {
    kprintf("Unimplemented\n");
    for (;;)
      ;
  }
  }

  return -1;
}

int pty_init() {

  VFSNode *new;
  VAttr attr = {.type = VFS_CHARDEVICE, .rdev = MKDEV(PTMX_MAJOR, PTMX_MINOR)};
  dev_root->ops->create(dev_root, &new, "/dev/ptmx", &attr);

  new->ops = &ptmx_ops;

  VFSNode *pts_dirnode;
  if (dev_root->ops->lookup(dev_root, &pts_dirnode, "/dev/pts/")) {
    VAttr attr = {.type = VFS_DIRECTORY};
    dev_root->ops->mkdir(dev_root, &pts_dirnode, "/dev/pts/", &attr);
  }

  return 0;
}
