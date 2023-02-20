#include "drivers/keyboard.h"
#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "libk/kmalloc.h"
#include "libk/ringbuffer.h"
#include <asm-generic/poll.h>
#include <fs/devfs.h>

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
  // VFSNode *input_dir;
  // VAttr attr = {.type = VFS_DIRECTORY};
  // if (dev_root->ops->mkdir(dev_root, &input_dir, "/dev/input/", &attr))
  //   return -1; // couldn't create input directory

  // initialize kbd for keyboard

  VFSNode *ps2keyboard;
  VAttr attr = (VAttr){.type = VFS_CHARDEVICE};
  if (dev_root->ops->create(dev_root, &ps2keyboard, "/dev/ps2keyboard", &attr))
    return -1;

  TmpNode *tnode = ps2keyboard->private_data;
  RingBuffer *input_rb = kmalloc(sizeof(RingBuffer));
  rb_init(input_rb, INPUT_BUFSIZE, sizeof(char));
  tnode->dev.cdev.private_data = input_rb;

  tnode->dev.cdev.fs = &input_ops;

  kbd_register_buffer(input_rb);

  return 0;
}
