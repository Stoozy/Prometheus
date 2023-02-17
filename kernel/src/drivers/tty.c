#include "cpu/idt.h"
#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "memory/vmm.h"
#include "proc/proc.h"
#include <abi-bits/linux/vt.h>
#include <asm-generic/ioctls.h>
#include <asm-generic/poll.h>
#include <drivers/tty.h>
#include <fs/devfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/ringbuffer.h>
#include <libk/util.h>
#include <stdint.h>
#include <string/string.h>

#include <abi-bits/asm/ioctls.h>
struct tty *gp_active_tty;
static ProcessQueue tty_wait_queue;

struct tty g_tty_table[MAX_TTYS];
static uint8_t g_tty_counter;
struct tty_ldisc g_ldiscs[MAX_TTYS];
struct tty_driver g_tty_drivers[MAX_TTYS];

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

ssize_t tty_read(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tmpnode = vn->private_data;
  struct tty *tty = tmpnode->dev.cdev.private_data;
  return tty->driver.read(tty, nbyte, buf);
}

ssize_t tty_write(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tmpnode = vn->private_data;
  struct tty *tty = tmpnode->dev.cdev.private_data;
  return tty->driver.write(tty, nbyte, buf);
}

int tty_ioctl(struct vnode *vp, uint64_t req, void *data, int fflag) {
  kprintf("tty_ioctl()\n");
  TmpNode *tty_tnode = vp->private_data;
  struct tty *tty = tty_tnode->dev.cdev.private_data;
  if (tty->driver.ioctl)
    return tty->driver.ioctl(tty, req, data);

  return -1;
}

int tty_poll(struct vnode *vp, int events) {
  kprintf("tty_poll()");
  for (;;)
    ;
  return -1;
}
/* initializes new device */

// struct tty *tty_init_dev(struct tty_driver driver, int minor) {
//   RingBuffer *in = kmalloc(sizeof(RingBuffer));
//   RingBuffer *out = kmalloc(sizeof(RingBuffer));

//   rb_init(in, TTY_BUFSIZE, sizeof(uint8_t));
//   rb_init(out, TTY_BUFSIZE, sizeof(uint8_t));

//   g_tty_table[g_tty_counter] =
//       (struct tty){.driver = driver, .ibuf = in, .obuf = out};

//   return &g_tty_table[g_tty_counter++];
// }

// static File *file_from_tty(struct tty *tty, int minor) {
//   File *file = kmalloc(sizeof(File));
//   file->size = TTY_BUFSIZE;
//   file->device = MKDEV(tty->driver.dev_major, minor);
//   file->fs = &tty_fops;
//   file->position = 0;
//   file->private_data = tty;

//   file->name = kmalloc(50);
//   sprintf(file->name, "tty%d\0", minor);

//   return file;
// }

// int tty_open(VFSNode *vn, const char *filename, int flags) {
//   kprintf("[TTY]  Called open on %s\n", filename);

//   // FIXME: hardcoded
//   int minor = 1;

//   /* // first search for it */
//   /* if (g_tty_table[minor].driver.dev_major) */
//   /*   return file_from_tty(&g_tty_table[minor], minor); */

//   struct tty *new_tty = tty_init_dev(default_tty_driver);
//   kprintf("Tty not found, making a new one\n");

//   gp_active_tty = new_tty;

//   return file_from_tty(new_tty, g_tty_counter);
// }

// void tty_close(struct file *file) {
//   // TODO
//   return;
// }

// size_t tty_read(struct file *file, size_t size, uint8_t *buf) {
//   struct tty *tty = file->private_data;
//   if (tty->driver.read)
//     return tty->driver.read(tty, size, buf);
//   else
//     return tty_default_read(tty, size, buf);
// }

// size_t tty_write(struct file *file, size_t size, uint8_t *buf) {
//   struct tty *tty = file->private_data;
//   if (tty) {
//     if (tty->driver.write) {
//       return tty->driver.write(tty, size, buf);
//     } else {
//       // kprintf("Write not implemented :(\n");
//       return tty_default_write(tty, size, buf);
//     }
//   }

//   return 0;
// }

// int tty_ioctl(struct file *file, uint32_t request, void *arg) {
//   struct tty *tty = file->private_data;
//   if (tty)
//     return tty->driver.ioctl(tty, request, arg);
//   return 0;
// }

// int tty_poll(struct file *file, struct pollfd *pfd, int timeout) {
//   struct tty *tty = file->private_data;

//   if (tty)
//     return tty->driver.poll(tty, pfd, timeout);

//   return 0;
// }

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

  for (; s < size; s++)
    if (!rb_pop(tty->ibuf, &buffer[s]))
      break;
    else
      echo(tty, buffer[s]);
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

int tty_default_poll(struct tty *tty, struct pollfd *fd, int timeout) {

  int evt = 0;
  if (fd->events & POLLIN) {

    enable_irq();
    if (tty->ibuf) {
      while (!tty->ibuf->len) {
      }
    }
    disable_irq();

    fd->revents |= POLLIN;
    evt++;
  } else if (fd->events & POLLOUT) {
    fd->revents |= POLLOUT;
    evt++;
  }

  return evt;
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
    if (tty->driver.ioctl && tty->driver.ioctl != tty_default_ioctl) {
      return tty->driver.ioctl(tty, request, arg);
    }
    return -1;
  }
  }

  return 0;
}

// int tty_register(dev_t dev, struct tty *tty, char *filename) {
//
//   //   CharacterDevice *cdev = kmalloc(sizeof(CharacterDevice));
//
//   //   cdev->private_data = tty;
//   //   cdev->dev = dev;
//   //   cdev->name = filename;
//
//   /*     struct cdev *cdev = alloc(sizeof(struct cdev)); */
//   /* cdev->fops = &tty_cdev_ops; */
//   /* cdev->private_data = tty; */
//   /* cdev->rdev = dev; */
//   /* if (cdev_register(cdev) == -1) { */
//   /*   free(cdev); */
//   /*   return -1; */
//   /* } */
//
//   return 0;
// }

VNodeOps tty_vnops = {.open = tty_open,
                      .read = tty_read,
                      .write = tty_write,
                      .ioctl = tty_ioctl,
                      .poll = tty_poll};

static struct tty_driver default_tty_driver = {
    .tty_table = &g_tty_table[0],
    .driver_name = "tty",
    .name = "tty",
    .dev_major = 5,
    .num_devices = MAX_TTYS,
    .read = tty_default_read,
    .write = tty_default_write,
    .flush_chars = tty_default_flush_chars,
    .write_room = tty_default_write_room,
    .set_termios = tty_default_set_termios,
    .set_ldisc = tty_default_set_ldisc,
    .poll = tty_default_poll,
    .ioctl = tty_default_ioctl};

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
  RingBuffer *in = kmalloc(sizeof(RingBuffer));
  RingBuffer *out = kmalloc(sizeof(RingBuffer));

  rb_init(in, TTY_BUFSIZE, sizeof(uint8_t));
  rb_init(out, TTY_BUFSIZE, sizeof(uint8_t));

  g_tty_table[minor] =
      (struct tty){.driver = default_tty_driver, .ibuf = in, .obuf = out};

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

// int tty_init() {
//   default_tty_driver =
//       (struct tty_driver){.tty_table = &g_tty_table[0],
//                           .driver_name = "tty",
//                           .name = "tty",
//                           .dev_major = 5,
//                           .num_devices = MAX_TTYS,
//                           .read = tty_default_read,
//                           .write = tty_default_write,
//                           .flush_chars = tty_default_flush_chars,
//                           .write_room = tty_default_write_room,
//                           .set_termios = tty_default_set_termios,
//                           .set_ldisc = tty_default_set_ldisc,
//                           .poll = tty_default_poll,
//                           .ioctl = tty_default_ioctl};

//   /* TODO */

//   CharacterDevice *chdev = kmalloc(sizeof(CharacterDevice));
//   chdev->name = kmalloc(256);
//   strcpy(chdev->name, "tty");
//   chdev->dev = MKDEV(TTY_MAJOR, 0);
//   chdev->fs = &tty_fops;

//   if (devfs_register_chardev(chdev) == -1) {
//     return -1;
//   }

//   return 0;
// }
