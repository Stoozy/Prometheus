#include "abi-bits/termios.h"
#include "cpu/idt.h"
#include "drivers/keyboard.h"
#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "memory/vmm.h"
#include "proc/proc.h"
#include <abi-bits/linux/vt.h>
#include <asm-generic/ioctls.h>
#include <asm-generic/poll.h>
#include <drivers/tty.h>
#include <fs/devfs.h>
#include <memory/slab.h>
#include <libk/kprintf.h>
#include <libk/ringbuffer.h>
#include <libk/util.h>
#include <stdint.h>
#include <string/string.h>

#include <abi-bits/asm/ioctls.h>
struct tty *gp_active_tty;

struct tty g_tty_table[MAX_TTYS];
static uint8_t g_tty_counter;
struct tty_ldisc g_ldiscs[MAX_TTYS];
struct tty_driver g_tty_drivers[MAX_TTYS];

size_t tty_default_read(struct tty *tty, size_t size, uint8_t *buffer);
size_t tty_default_write(struct tty *tty, size_t size, uint8_t *buffer);
size_t tty_default_write_room(struct tty *tty);
int tty_default_set_termios(struct tty *tty, struct termios tios);
int tty_default_set_ldisc(struct tty *tty, struct tty_ldisc ldisc);
int tty_default_poll(struct tty *tty, int events);
void tty_default_flush_chars(struct tty *tty);
int tty_default_ioctl(struct tty *tty, uint64_t request, void *arg);

static struct tty_driver default_tty_driver = {
    .tty_table = &g_tty_table[0],
    .driver_name = "tty",
    .name = "tty",
    .dev_major = TTY_MAJOR,
    .num_devices = MAX_TTYS,
    .read = tty_default_read,
    .write = tty_default_write,
    .flush_chars = tty_default_flush_chars,
    .write_room = tty_default_write_room,
    .set_termios = tty_default_set_termios,
    .set_ldisc = tty_default_set_ldisc,
    .poll = tty_default_poll,
    .ioctl = tty_default_ioctl};

int tty_create(VFSNode *dvn, VFSNode **out, const char *name, VAttr *attr) {
  kprintf("tty_create()");
  for (;;)
    ;
  return -1;
}
int tty_getattr(VFSNode *vn, VAttr *out) {

  kprintf("tty_getattr()");
  for (;;)
    ;
  return -1;
}
int tty_lookup(VFSNode *dvn, VFSNode **out, const char *name) {

  kprintf("tty_lookup()");
  for (;;)
    ;
  return -1;
}

int tty_open(File *file, VFSNode *vn, int mode) {
  kprintf("tty_open()");
  vn->refcnt++;

  file->vn = vn;
  file->pos = 0;
  file->refcnt = 1;

  return 0;
}

ssize_t tty_read(File *file, VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tmpnode = vn->private_data;
  struct tty *tty = tmpnode->dev.cdev.private_data;
  if (tty->driver.read)
    return tty->driver.read(tty, nbyte, buf);

  return tty_default_read(tty, nbyte, buf);
}

ssize_t tty_write(File *file, VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tmpnode = vn->private_data;
  struct tty *tty = tmpnode->dev.cdev.private_data;
  if (tty->driver.write)
    return tty->driver.write(tty, nbyte, buf);

  return tty_default_write(tty, nbyte, buf);
}

int tty_ioctl(struct vnode *vp, uint64_t req, void *data, int fflag) {
  kprintf("tty_ioctl()\n");
  TmpNode *tty_tnode = vp->private_data;
  struct tty *tty = tty_tnode->dev.cdev.private_data;
  if (tty->driver.ioctl)
    return tty->driver.ioctl(tty, req, data);

  return tty_default_ioctl(tty, req, data);
}

int tty_poll(struct vnode *vp, int events) {
  kprintf("tty_poll()");
  TmpNode *tty_tnode = vp->private_data;
  struct tty *tty = tty_tnode->dev.cdev.private_data;
  if (tty->driver.poll) {
    return tty->driver.poll(tty, events);
  }

  return tty_default_poll(tty, events);
}

static void echo(struct tty *tty, uint8_t val) {
  disable_irq();
  rb_push(tty->obuf, &val);
  enable_irq();
}

size_t tty_default_read(struct tty *tty, size_t size, uint8_t *buffer) {

  kprintf("[TTY]  Default read called for %d bytes\n", size);

  size_t s = 0;

read:
  disable_irq();

  for (; s < size; s++) {
    if (!rb_pop(tty->ibuf, &buffer[s]))
      break;
  }
  enable_irq();
  /* extern ProcessControlBlock * running; */
  if (s == 0) {
    /* extern ProcessQueue kbd_wait_queue; */
    /* pqueue_push(&kbd_wait_queue, running); */
    /* block_process(running, WAITING); */

    goto read;
  }

  // should be in line disc
  return s;
}

size_t tty_default_write(struct tty *tty, size_t size, uint8_t *buffer) {
  kprintf("[TTY]  Default write called\n");
  kprintf("Data: ");

  for (int i = 0; i < size; i++) {
    if (!rb_push(tty->obuf, &buffer[i]))
      for (;;)
        kprintf("Couldn't write to buffer :(\n");

    kprintf("%c", ((char *)tty->obuf->buffer)[i]);
  }

  kprintf("\n");
  kprintf("[TTY] New length is %d\n", tty->obuf->len);

  if (tty->driver.flush_chars)
    tty->driver.flush_chars(tty);

  kprintf("[TTY] Returning %ld\n", size);
  return size;
}

void tty_default_put_char(struct tty *tty, char c) {
  for (;;)
    kprintf("Put char called\n");
}

void tty_default_flush_chars(struct tty *tty) {
  // TODO

  char ch;
  while (rb_pop(tty->obuf, &ch))
    ;

  /* for (;;) */
  /*   kprintf("default flush chars called\n"); */
}

size_t tty_default_write_room(struct tty *tty) { return 0; }

int tty_default_set_termios(struct tty *tty, struct termios tios) {
  tty->tios = tios;
  return 0;
}

int tty_default_set_ldisc(struct tty *tty, struct tty_ldisc ldisc) {
  tty->ldisc = ldisc;
  return 0;
}

int tty_default_poll(struct tty *tty, int events) {

  int revents = 0;
  if (events & POLLIN) {
    if (tty->ibuf->len)
      revents |= POLLIN;
  } else if (events & POLLOUT) {
    revents |= POLLOUT;
  }

  return revents;
}

int tty_default_ioctl(struct tty *tty, uint64_t request, void *arg) {
  // TODO

  switch (request) {
  case TIOCSCTTY: {
    gp_active_tty = tty;
    return 0;
  }
  case TIOCSWINSZ: {
    struct winsize *wsize = arg;
    tty->wsize = *wsize;
    return 0;
  }
  case VT_RELDISP:
  case VT_SETMODE:
    return 0;
  default: {
    kprintf("Unknown ioctl req %d\n", request);
    for (;;)
      ;
  }
  }

  return 0;
}

VNodeOps tty_vnops = {.open = tty_open,
                      .read = tty_read,
                      .write = tty_write,
                      .ioctl = tty_ioctl,
                      .poll = tty_poll};

int tty_init_node(VFSNode *tty_node) {
  if (!tty_node)
    return -1;

  // init fs-related things
  TmpNode *tty_tmp_node = tty_node->private_data;
  int minor = MINOR(tty_tmp_node->dev.cdev.rdev);
  tty_tmp_node->dev.cdev.private_data = &g_tty_table[minor];
  // tty_tmp_node->dev.cdev.fs = &tty_vnops;
  tty_node->ops = &tty_vnops;

  // init tty internal structures
  RingBuffer *in = kmem_alloc(sizeof(RingBuffer));
  RingBuffer *out = kmem_alloc(sizeof(RingBuffer));

  rb_init(in, TTY_BUFSIZE, sizeof(uint8_t));
  rb_init(out, TTY_BUFSIZE, sizeof(uint8_t));

  g_tty_table[minor] =
      (struct tty){.driver = default_tty_driver, .ibuf = in, .obuf = out};

  kbd_register_buffer(in);
  return 0;
}

int tty_init() {
  // create /dev/tty[0-7]
  // set their operations to ttyfs ops

  char name[256];
  for (int i = 0; i < MAX_TTYS; i++) {
    sprintf(name, "/dev/tty%d", i);
    VFSNode *tty_node;
    VAttr attr = {.rdev = MKDEV(TTY_MAJOR, i), .type = VFS_CHARDEVICE};
    if (dev_root->ops->create(dev_root, &tty_node, name, &attr)) {
      kprintf("Couldn't create %s\n", name);
      for (;;)
        ;
    }

    if (tty_init_node(tty_node)) {
      kprintf("Couldn't initialize %s\n", name);
      for (;;)
        ;
    }
  }

  return 0;
}
