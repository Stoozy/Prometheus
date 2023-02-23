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

extern ProcessControlBlock *running;
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

  if (name[0] == '.' && name[1] == '/') {

    kprintf("opening %s\n", name);
    for (;;)
      ;
  }
  kprintf("opening %s\n", name);

  char *lookup_path = (char *)name;
  VFSNode *vnode;
  if (strcmp(name, ".") == 0) {
    lookup_path = strdup(running->cwd);
    kprintf("lookup path is %s\n", lookup_path);
    goto got_path;
  }

  if (strcmp(name, "..") == 0) {
    extern ProcessControlBlock *running;
    lookup_path = get_parent_dir(running->cwd);
    goto got_path;
  }

  if (name[0] != '/') {
    lookup_path = kmalloc(strlen(running->cwd) + strlen(name) + 1);
    if (strcmp(running->cwd, "/") == 0)
      sprintf(lookup_path, "/%s", name);
    else
      sprintf(lookup_path, "%s/%s", running->cwd, name);
    kprintf("Looking up %s \n", lookup_path);
    goto got_path;
  }

got_path:
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

  return file->vn->ops->read(file, file->vn, buffer, size, file->pos);
}

ssize_t vfs_write(File *file, void *buffer, size_t size) {
  return file->vn->ops->write(file, file->vn, buffer, size, file->pos);
}

int vfs_close(File *file) {
  if (--file->refcnt == 0) {
    kfree(file);
  }
  return 0;
}

int vfs_stat(const char *path, VFSNodeStat *vns) {
  char *lookup_path;

  if (strcmp(path, ".") == 0) {
    // lookup cwd
    lookup_path = running->cwd;
    goto got_path;
  }

  if (strcmp(path, "..") == 0) {
    // lookup cwd
    extern ProcessControlBlock *running;
    lookup_path = get_parent_dir(running->cwd);
    goto got_path;
  }

  if (path[0] != '/') {
    lookup_path = kmalloc(strlen(running->cwd) + strlen(path) + 1);
    if (strcmp(running->cwd, "/") == 0)
      sprintf(lookup_path, "/%s", path);
    else
      sprintf(lookup_path, "%s/%s", running->cwd, path);
    kprintf("Looking up %s \n", lookup_path);
    goto got_path;
  }

  lookup_path = (char *)path;
got_path:
  kprintf("lookup path %s\n", path);
  VFSNode *lookup;
  if (strcmp(path, "/") != 0) {
    kprintf("Looking up %s\n", lookup_path);
    if (root_vnode->ops->lookup(root_vnode, &lookup, lookup_path)) {
      kprintf("Couldn't find vnode for %s\n", lookup_path);
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

int vfs_mkdir(const char *path, mode_t mode) {
  if (strcmp(path, ".") == 0)
    return -1;

  if (strcmp(path, "..") == 0)
    return -1;

  kprintf("Creating directory %s\n", path);
  char *absolute_path;
  char *parent_path;
  if (path[0] == '.' && path[1] == '/') {
    path++;
    absolute_path = kmalloc(strlen(running->cwd) + strlen(path) + 2);
    if (path[strlen(path) - 1] != '/')
      sprintf(absolute_path, "%s%s/", running->cwd, path);
    else
      sprintf(absolute_path, "%s%s", running->cwd, path);
    kprintf("Absolute path is %s\n", absolute_path);
    parent_path = running->cwd;
  } else {
    absolute_path = (char *)path;
    parent_path = get_parent_dir(path);
  }

  kprintf("absolute path is %s\n", absolute_path);
  kprintf("parent dir is %s\n", parent_path);
  VFSNode *parent_node;
  if (root_vnode->ops->lookup(root_vnode, &parent_node, parent_path)) {
    kprintf("Couldn't find parent dir");
    for (;;)
      ;

    return -1;
  }

  VFSNode *child;
  VAttr attr = {.mode = mode, .type = VFS_DIRECTORY};
  if (parent_node->ops->mkdir(parent_node, &child, absolute_path, &attr)) {
    kprintf("Couldn't create directory");
    for (;;)
      ;
    return -1;
  }

  kprintf("created directory %s (vnode @ 0x%x)\n", absolute_path, child);
  return 0;
}
