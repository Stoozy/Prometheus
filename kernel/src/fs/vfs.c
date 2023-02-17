#include "abi-bits/fcntl.h"
#include "fs/tmpfs.h"
#include "memory/vmm.h"
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <string/string.h>

#include <proc/proc.h>
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
  kprintf("opening %s\n", name);

  VFSNode *vnode;
  if (root_vnode->ops->lookup(root_vnode, &vnode, name)) {
    if (!(flags & O_CREAT)) {
      return NULL;
    }

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
    if (parent->ops->create(parent, &vnode, name, &attr)) {
      kprintf("Failed to create %s\n", name);
      return NULL; // couldn't create file
    }
  }

  if (vnode->ops->open(file, vnode, flags))
    return NULL;

  return file;
}

ssize_t vfs_read(File *file, void *buffer, size_t size) {
  kprintf("file is @ 0x%llx\n", file);
  kprintf("vnode is @ 0x%llx\n", file->vn);
  TmpNode *tnode = file->vn->private_data;
  kprintf("Calling read on %s for %lu bytes\n", tnode->name, size);
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

int vfs_stat(const char *path, VFSNodeStat *vns) {
  if (strcmp(path, ".") == 0) {
    // lookup cwd
    extern ProcessControlBlock *running;
    path = running->cwd;
  }

  if (strcmp(path, "..") == 0) {
    // lookup cwd
    extern ProcessControlBlock *running;
    path = get_parent_dir(running->cwd);
  }

  VFSNode *lookup;
  if (strcmp(path, "/") != 0) {

    kprintf("Looking up %s\n", path);
    if (root_vnode->ops->lookup(root_vnode, &lookup, path)) {
      kprintf("Couldn't find vnode for %s\n", path);
      return -1;
    }
  } else
    lookup = root_vnode;

  vns->filesize = lookup->size;
  vns->inode = (ino_t)lookup->private_data;
  vns->type = lookup->type;
  if (lookup->type == VFS_CHARDEVICE || lookup->type == VFS_BLOCKDEVICE) {
    TmpNode *tnode = lookup->private_data;
    vns->rdev = tnode->dev.cdev.rdev;
  }

  return 0;
}
