#include "fs/vfs.h"
#include "libk/kprintf.h"

VFSNode *dev_root = NULL;
int devfs_init() {
  VFSNode *devnode;
  if (root_vnode->ops->lookup(root_vnode, &devnode, "/dev/")) {
    // `/dev/` not found, create it ...

    VAttr attr = {.type = VFS_DIRECTORY};
    if (root_vnode->ops->mkdir(root_vnode, &devnode, "/dev/", &attr)) {
      kprintf("Couldn't create /dev/");
      return -1;
    }
  }

  dev_root = devnode;
  return 0;
}