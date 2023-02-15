#include "abi-bits/fcntl.h"
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <string/string.h>

VFS vfs_root;
VFSNode *root_vnode;

static char *get_parent_dir(const char *path) {
  char *parent = kmalloc(strlen(path));
  strcpy(parent, path);

  size_t parent_len = strlen(parent);
  if (parent[parent_len - 1] == '/')
    parent[parent_len - 1] = '\0';

  int i = parent_len - 1;
  while (parent[i] != '/') {
    i--;
  }

  parent[i + 1] = '\0';

  return parent;
}

File *vfs_open(const char *name, int flags) {
  File *file = kmalloc(sizeof(File));

  VFSNode *vnode;
  if (root_vnode->ops->lookup(root_vnode, &vnode, name)) {
    if (!(flags & O_CREAT))
      return NULL;

    // attempt to create file...

    VFSNode *parent;
    char *parent_name = get_parent_dir(name);
    kprintf("Looking up %s\n", parent_name);
    if (root_vnode->ops->lookup(root_vnode, &parent, parent_name)) {
      kprintf("Couldn't find parent %s\n", parent_name);
      return NULL; // give up (missing parent dir)
    }

    kprintf("Got parent at %s\n", parent_name);
    VAttr attr = {.type = VFS_FILE};
    if (parent->ops->create(parent, &vnode, name, &attr))
      return NULL; // couldn't create file
  }

  root_vnode->ops->open(vnode, flags);

  file->vn = vnode;
  file->pos = 0;
  file->refcnt = 1;

  return file;
}

ssize_t vfs_read(File *file, void *buffer, size_t size) {
  return file->vn->ops->read(file->vn, buffer, size, file->pos);
}

ssize_t vfs_write(File *file, void *buffer, size_t size) {
  return file->vn->ops->write(file->vn, buffer, size, file->pos);
}

int vfs_close(File *file) {
  if (--file->refcnt == 0) {
    kfree(file);
  }
  return 0;
}