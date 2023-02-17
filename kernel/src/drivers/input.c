// #include "drivers/keyboard.h"
// #include "fs/devfs.h"
// #include "libk/ringbuffer.h"
// #include <fs/vfs.h>
// #include <libk/kmalloc.h>
// #include <libk/kprintf.h>
// #include <libk/ringbuffer.h>
// #include <string/string.h>

#include "drivers/keyboard.h"
#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "libk/kmalloc.h"
#include "libk/ringbuffer.h"
#include <asm-generic/poll.h>
#include <fs/devfs.h>
// File *input_open(const char *filename, int flags);
// void input_close(struct file *);
// size_t input_read(struct file *, size_t, uint8_t *buf);
// int input_poll(struct file *, struct pollfd *, int timeout);

#define INPUT_BUFSIZE 4096

int input_open(File *file, VFSNode *vn, int mode);
ssize_t input_read(VFSNode *vn, void *buf, size_t nbyte, off_t off);
int input_poll(VFSNode *vp, int events);

VNodeOps input_ops = {
    .open = input_open, .read = input_read, .poll = input_poll};

int input_open(File *file, VFSNode *vn, int mode) {
  vn->refcnt++;
  return 0;
}

ssize_t input_read(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *input_tnode = vn->private_data;
  RingBuffer *rb = input_tnode->dev.cdev.private_data;

  char ch;
  size_t ret = 0;
  while (rb_pop(rb, &ch)) {
    ((char *)buf)[ret++] = ch;
  }

  return ret;
}

int input_poll(VFSNode *vp, int events) {
  TmpNode *input_tnode = vp->private_data;
  RingBuffer *rb = input_tnode->dev.cdev.private_data;

  int revents = 0;
  if (events & POLLIN) {
    if (rb->len) {
      revents |= POLLIN;
    }
  }

  if (events & POLLOUT) {
    revents |= POLLOUT;
  }

  return revents;
}

int input_init() {
  VFSNode *input_dir;
  VAttr attr = {.type = VFS_DIRECTORY};
  if (dev_root->ops->mkdir(dev_root, &input_dir, "/dev/input/", &attr))
    return -1; // couldn't create input directory

  // initialize kbd for keyboard

  VFSNode *event0;
  attr = (VAttr){.type = VFS_CHARDEVICE};
  if (dev_root->ops->create(input_dir, &event0, "/dev/input/event0", &attr))
    return -1;

  TmpNode *tnode = event0->private_data;
  RingBuffer *input_rb = kmalloc(sizeof(RingBuffer));
  rb_init(input_rb, INPUT_BUFSIZE, sizeof(char));
  tnode->dev.cdev.private_data = input_rb;

  tnode->dev.cdev.fs = &input_ops;

  kbd_register_buffer(input_rb);

  return 0;
}

// File *input_open(const char *filename, int flags) {
//   if (strcmp(filename, "input/event0") != 0)
//     return NULL;

//   struct input_dev *idev = kmalloc(sizeof(struct input_dev));
//   rb_init(&idev->buffer, INPUT_BUFSIZE, sizeof(char));

//   kbd_register_buffer(&idev->buffer);

//   File *file = kmalloc(sizeof(File));
//   file->name = kmalloc(256);
//   sprintf(file->name, "input/event0");

//   file->device = MKDEV(13, 64);
//   file->fs = &input_fs;
//   file->private_data = idev;

//   return file;
// }

// void input_close(struct file *file) {
//   for (;;)
//     kprintf("TODO: input_close\n");
// }

// size_t input_read(struct file *file, size_t size, uint8_t *buf) {
//   struct input_dev *idev = file->private_data;

//   char ch;
//   size_t ret = 0;
//   while (rb_pop(&idev->buffer, &ch)) {
//     buf[ret++] = ch;
//   }

//   return ret;
// }

// int input_poll(struct file *file, struct pollfd *pfd, int timeout) {
//   struct input_dev *idev = file->private_data;

//   int evt = 0;

//   if (pfd->events & POLLIN) {
//     if (idev->buffer.len) {
//       pfd->revents |= POLLIN;
//       evt++;
//     }
//   }

//   if (pfd->events & POLLOUT) {
//     pfd->revents |= POLLOUT;
//     evt++;
//   }

//   return evt;
// }
