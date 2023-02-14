#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <string/string.h>

VFS vfs_root;
VFSNode *root_vnode;

File *vfs_open(const char *name, int flags) {
  File *file = kmalloc(sizeof(File));

  struct componentname cn = {.cn_nameptr = name, .cn_namelen = strlen(name)};
  VFSNode *vnode;
  root_vnode->ops->lookup(root_vnode, &vnode, &cn);

  if (!vnode)
    return NULL;

  root_vnode->ops->open(vnode, flags);

  file->vn = vnode;
  file->pos = 0;
  file->refcnt = 1;

  return file;
}

ssize_t vfs_read(File *file, void *buffer, size_t size) {
  return file->vn->ops->read(file->vn, buffer, size, file->pos);
}

ssize_t vfs_write(File *file, void *buffer, size_t size) {}

int vfs_close(File *file) {
  if (--file->refcnt == 0) {
    kfree(file);
  }
  return 0;
}