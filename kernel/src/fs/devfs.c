#include "fs/vfs.h"
#include "libk/kprintf.h"

VFSNode *dev_root = NULL;
// TmpNode *dev_tmp_root = NULL;

// static int devfs_root(VFS *vfs, VFSNode **out) {
//   TmpNode *tmp_root = (TmpNode *)vfs->private_data;
//   *out = tmp_root->vnode;
//   return 0;
// }

// static int devfs_vget(VFS *vfs, VFSNode **out, ino_t inode) {
//   TmpNode *node = (TmpNode *)inode;

//   if (node->vnode != NULL) {
//     node->vnode->refcnt++;
//     *out = node->vnode;
//     return 0;
//   }

//   VFSNode *vnode = kmalloc(sizeof(VFSNode));
//   node->vnode = vnode;
//   vnode->refcnt = 1;

//   vnode->type = node->attr.type;
//   vnode->ops = &devfs_vnops;
//   vnode->vfs = vfs;
//   vnode->vfs_mountedhere = NULL;
//   vnode->isroot = false;
//   vnode->size = node->attr.size;

//   vnode->private_data = node;
//   *out = vnode;
//   return 0;
// }

// static int devfs_create(VFSNode *dvn, VFSNode **out, const char *name,
//                         VAttr *attr) {
//   kprintf("Calling devfs create on %s\n", name);
//   TmpNode *new = tmakenode(dev_tmp_root, name, attr);

//   if (!new)
//     kprintf("Couldn't make node :(\n");

//   return dvn->vfs->ops->vget(dvn->vfs, out, (ino_t) new);
// }
// static int devfs_getattr(VFSNode *vn, VAttr *out);
// static int devfs_lookup(VFSNode *dvn, VFSNode **out, const char *name) {
//   return -1;
// }

// static int devfs_mkdir(struct vnode *dvp, struct vnode **vpp, const char
// *cnp,
//                        struct vattr *vap) {
//   return -1;
// }
// static int devfs_mknod(struct vnode *dvp, struct vnode **vpp, const char
// *cnp,
//                        struct vattr *vap);
// static int devfs_open(VFSNode *vn, int mode) { return -1; }
// static int devfs_readdir(VFSNode *dvn, void *buf, size_t nbyte,
//                          size_t *bytesRead, off_t seqno);
// static ssize_t devfs_read(VFSNode *vn, void *buf, size_t nbyte, off_t off);
// static ssize_t devfs_write(VFSNode *vn, void *buf, size_t nbyte, off_t off);

// VNodeOps devfs_vnops = {.create = devfs_create,
//                         .open = devfs_open,
//                         .mkdir = devfs_mkdir,
//                         .lookup = devfs_lookup};

// VFSOps devfs_vfsops = {.root = devfs_root, .vget = devfs_vget};

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

// int devfs_init() {
//   kprintf("Intializing devfs...\n");

//   devfs.ops = &devfs_vfsops;

//   VAttr attr = {.type = VFS_DIRECTORY, .size = 0};
//   if (root_vnode->ops->mkdir(root_vnode, &dev_root, "/dev/", &attr)) {
//     kprintf("Couldn't create dev directory\n");
//     return -1;
//   }

//   dev_root->isroot = true;
//   dev_root->vfs_mountedhere = &devfs;
//   dev_root->size = 0;
//   dev_root->vfs = &vfs_root;
//   dev_root->ops = &devfs_vnops;

//   dev_tmp_root = dev_root->private_data;
//   devfs.private_data = dev_tmp_root;

//   TAILQ_INIT(&dev_tmp_root->dir.dirents);

//   return 0;
// }