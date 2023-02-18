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

  char *lookup_path = (char *)name;
  VFSNode *vnode;

  if (strcmp(name, ".") == 0) {
    extern ProcessControlBlock *running;
    lookup_path = strdup(running->cwd);
    kprintf("lookup path is %s\n", lookup_path);
  }

  if (strcmp(name, "..") == 0) {
    extern ProcessControlBlock *running;
    lookup_path = get_parent_dir(running->cwd);
  }

  if (root_vnode->ops->lookup(root_vnode, &vnode, lookup_path)) {
    if (!(flags & O_CREAT)) {
      kprintf("can't create file %s\n", lookup_path);
      kprintf("lookup is @ 0x%lx\n", root_vnode->ops);
      return NULL;
    }

    // attempt to create file...

    kprintf("Creating %s\n", lookup_path);
    VFSNode *parent;
    char *parent_name = get_parent_dir(lookup_path);
    kprintf("Looking up %s\n", parent_name);
    if (root_vnode->ops->lookup(root_vnode, &parent, parent_name)) {
      kprintf("Couldn't find parent %s\n", parent_name);
      return NULL; // give up (missing parent dir)
    }

    kprintf("Got parent at %s\n", parent_name);
    VAttr attr = {.type = VFS_FILE};
    if (parent->ops->create(parent, &vnode, lookup_path, &attr)) {
      kprintf("Failed to create %s\n", lookup_path);
      return NULL; // couldn't create file
    }
  }

  kprintf("vfs_open(): vnode is @ 0x%x\n", vnode);

  if (vnode->ops->open(file, vnode, flags))
    return NULL;

  return file;
}

int vfs_readdir(File *file, DirectoryEntry *buf) {
  VFSNode *vn = file->vn;
  return vn->ops->readdir(vn, buf, sizeof(DirectoryEntry), NULL, file->pos++);
}

ssize_t vfs_read(File *file, void *buffer, size_t size) {
  // kprintf("file is @ 0x%llx\n", file);
  // kprintf("vnode is @ 0x%llx\n", file->vn);
  // TmpNode *tnode = file->vn->private_data;
  // kprintf("Calling read on %s for %lu bytes\n", tnode->name, size);
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
  kprintf("lookup path %s\n", path);

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
