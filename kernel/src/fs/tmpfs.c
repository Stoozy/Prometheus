#include "fs/devfs.h"
#include "libk/util.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "misc/ssfn.h"
#include "proc/proc.h"
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <memory/slab.h>
#include <libk/kprintf.h>
#include <string/string.h>

#include <sys/queue.h>

static int tmpfs_mount(VFS *vfs, const char *path, void *data) {
  TmpNode *tmp_root_node = kmem_alloc(sizeof(TmpNode));
  VFSNode *tmp_root_vnode;

  TAILQ_INIT(&tmp_root_node->dir.dirents);

  tmp_root_node->attr.type = VFS_DIRECTORY;
  tmp_root_node->vnode = NULL;

  tmpfs_vfsops.vget(vfs, &tmp_root_vnode, (ino_t)tmp_root_node);
  tmp_root_vnode->isroot = true;
  vfs->private_data = tmp_root_node;
  tmp_root_node->vnode = tmp_root_vnode;

  root_vnode = tmp_root_vnode;

  tmp_root_node->name = strdup("/");
  return 0;
}

static int tmpfs_root(VFS *vfs, VFSNode **out) {
  TmpNode *tmp_root = (TmpNode *)vfs->private_data;
  *out = tmp_root->vnode;
  return 0;
}

static int tmpfs_vget(VFS *vfs, VFSNode **out, ino_t inode) {
  TmpNode *node = (TmpNode *)inode;

  if (node->vnode != NULL) {
    node->vnode->refcnt++;
    *out = node->vnode;
    return 0;
  }

  VFSNode *vnode = kmem_alloc(sizeof(VFSNode));
  node->vnode = vnode;
  vnode->refcnt = 1;

  vnode->ops = &tmpfs_vnops;
  vnode->vfs = vfs;
  vnode->vfs_mountedhere = NULL;
  vnode->isroot = false;

  vnode->stat.type = node->attr.type;
  vnode->stat.filesize = node->attr.size;
  vnode->stat.inode = inode;

  vnode->private_data = node;
  *out = vnode;
  return 0;
}

VFSOps tmpfs_vfsops = {
    .mount = tmpfs_mount, .root = tmpfs_root, .vget = tmpfs_vget};

static int tmpfs_lookup(VFSNode *dvn, VFSNode **out, const char *name) {

  // kprintf("Looking up %s\n", name);
  TmpNode *tnode = dvn->private_data;

  if (strcmp(name, "/") == 0) {
    VFSNode *ret;
    tmpfs_root(&vfs_root, &ret);
    kprintf("Found root at 0x%p\n", ret);
    *out = ret;
    return 0;
  }

  if (strcmp(name, ".") == 0) {
    dvn->refcnt++;
    *out = dvn;
    return 0;
  }

  if (strcmp(name, "..") == 0) {
    if (tnode->dir.parent) {
      *out = tnode->dir.parent->vnode;
      return 0;
    }
  }

  struct tmp_dir_q *new_dir = kmem_alloc(sizeof(struct tmp_dir_q));
  TAILQ_INIT(new_dir);

  struct tmpfs_dirent *dirent;
loop:
  TAILQ_FOREACH(dirent, &tnode->dir.dirents, entries) {
    TmpNode *next_tnode = dirent->inode;

    if (strcmp(next_tnode->name, name) == 0) {
      *out = next_tnode->vnode;
      return 0;
    }

    // in case of directory
    char path[strlen(name) + 1];
    sprintf(path, "%s/", name);
    if (strcmp(next_tnode->name, path) == 0) {
      *out = next_tnode->vnode;
      return 0;
    }

    // kprintf("Checking if %s starts with %s\n", next_tnode->name, name);
    if (strncmp(next_tnode->name, name, strlen(next_tnode->name)) == 0 &&
        next_tnode->name[strlen(next_tnode->name) - 1] ==
            '/' /* make sure it's actually a directory*/) {
      // kprintf("Found matching dir %s\n", next_tnode->name);
      tnode = dirent->inode;
      // TODO: follow mountpoints
      goto loop;
    }
    // kprintf("\n");
  }

  return -1;
}

TmpNode *tmakenode(TmpNode *dtn, const char *name, struct vattr *vap) {

  TmpNode *tnode = kmem_alloc(sizeof(TmpNode));
  struct tmpfs_dirent *dent = kmem_alloc(sizeof(struct tmpfs_dirent));

  dent->filename = strdup(name + strlen(dtn->name));
  dent->inode = tnode;

  tnode->name = strdup(name);

  if (vap) {
    tnode->attr = *vap;
  } else {
    memset(&tnode->attr, 0, sizeof(tnode->attr));
  }

  tnode->attr.size = 0;
  tnode->vnode = NULL;

  switch (vap->type) {
  case VFS_DIRECTORY: {
    TAILQ_INIT(&tnode->dir.dirents);

    tnode->dir.parent = dtn;
    dent->filename[strlen(dent->filename) - 1] = '\0';

    break;
  }
  case VFS_CHARDEVICE: {
    tnode->dev.cdev.rdev = vap->rdev;
    tnode->dev.cdev.filename = strdup(name);
    tnode->dev.cdev.size = vap->size;

    break;
  }
  case VFS_FILE: {
    TAILQ_INIT(&tnode->file.anonmap);
    break;
  }
  case VFS_SYMLINK: {
    break; // do nothing for now, `tmpfs_symlink` handles the rest
  }
  default:
    kprintf("tmpfs::mknod() unhandled type %d\n", vap->type);
    for (;;)
      ;
  }

  TAILQ_INSERT_TAIL(&dtn->dir.dirents, dent, entries);

  return tnode;
}

static int tmpfs_create(VFSNode *dvn, VFSNode **out, const char *name,
                        VAttr *vap) {

  TmpNode *new = tmakenode(dvn->private_data, name, vap);

  if (!new)
    kprintf("Couldn't make node :(\n");

  return dvn->vfs->ops->vget(dvn->vfs, out, (ino_t) new);
}

static int tmpfs_getattr(VFSNode *dvn, VAttr *out) { return -1; }

static int tmpfs_open(File *file, VFSNode *vn, int mode) {

  kprintf("tmpfs_open()\n");

  if (!vn)
    for (;;)
      kprintf("Called open on NULL vfs node!!\n");

  if (vn->stat.type == VFS_SYMLINK) {
    TmpNode *src_tnode = vn->private_data;
    VFSNode *tgt_vnode;

    kprintf("Looking up %s for symlink %s\n", src_tnode->link, src_tnode->name);
    if (tmpfs_lookup(root_vnode, &tgt_vnode, src_tnode->link)) {
      kprintf("Couldn't find %s\n", src_tnode->link);
      return -1;
    }

    tgt_vnode->refcnt++;
    file->vn = tgt_vnode;
    file->pos = 0;
    file->refcnt = 1;

    return 0;
  }

  vn->refcnt++;

  file->vn = vn;
  file->pos = 0;
  file->refcnt = 1;

  return 0;
}

static int tmpfs_readdir(VFSNode *dvn, void *buf, size_t nbyte,
                         size_t *bytesRead, off_t seqno) {
  kprintf("tmpfs_readdir()");

  TmpNode *dirnode = dvn->private_data;

  DirectoryEntry *entry = buf;
  struct tmpfs_dirent *dirent;
  int i = 0;
  TAILQ_FOREACH(dirent, &dirnode->dir.dirents, entries) {
    if (i == seqno) {
      entry->d_ino = (ino_t)dirent->inode;
      strcpy(entry->d_name, dirent->filename);
      entry->d_reclen = sizeof(DirectoryEntry);
      entry->d_type = dirent->inode->attr.type;
      entry->d_off = i;
      return 0;
    }
    i++;
  }

  return -1;
}

int tmpfs_mkdir(VFSNode *dvp, VFSNode **out, const char *name,
                struct vattr *vap) {

  TmpNode *new = tmakenode(dvp->private_data, name, vap);
  if (!new)
    kprintf("Couldn't make node :(\n");

  return dvp->vfs->ops->vget(dvp->vfs, out, (ino_t) new);
}

static ssize_t tmpfs_read(File *file, VFSNode *vn, void *buf, size_t nbyte,
                          off_t off) {
  TmpNode *tnode = vn->private_data;
  if (tnode->attr.type == VFS_CHARDEVICE) {
    return tnode->dev.cdev.fs->read(file, vn, buf, nbyte, off);
  }
  kprintf("Reading %lu bytes from %s\n", nbyte, tnode->name);

  if (file->pos > tnode->attr.size)
    return 0;

  if (nbyte + off > tnode->attr.size && tnode->attr.size) {
    kprintf("Reading outside of file ... ");
    // do a partial read
    size_t bytes_to_read = tnode->attr.size - off;

    if (tnode->attr.size != 0) {
      // read entire file
      void *start = TAILQ_FIRST(&tnode->file.anonmap)->page + off;
      memcpy(buf, start, bytes_to_read);
      file->pos += bytes_to_read;
      return bytes_to_read;
    }
  }

  void *start = TAILQ_FIRST(&tnode->file.anonmap)->page + off;
  memcpy(buf, start, nbyte);
  file->pos += nbyte;

  return nbyte;
}

static ssize_t tmpfs_write(File *file, VFSNode *vn, void *buf, size_t nbyte,
                           off_t off) {

  TmpNode *tnode = vn->private_data;

  // kprintf("writing to %s\n", tnode->name);

  if (nbyte + off > tnode->attr.size) {
    int pages_needed =
        DIV_ROUND_UP((nbyte + off) - tnode->attr.size, PAGE_SIZE);
    void *pages = PAGING_VIRTUAL_OFFSET + pmm_alloc_blocks(pages_needed);
    struct anon *na = kmem_alloc(sizeof(struct anon));
    na->page = pages;
    TAILQ_INSERT_TAIL(&tnode->file.anonmap, na, entries);

    tnode->attr.size = nbyte + off; // new size
    tnode->vnode->stat.filesize = tnode->attr.size;
  }

  void *start = TAILQ_FIRST(&tnode->file.anonmap)->page + off;
  memcpy(start, buf, nbyte);
  return nbyte;
}

int tmpfs_ioctl(VFSNode *vp, uint64_t req, void *data, int fflag) {
  if (vp->stat.type != VFS_CHARDEVICE) {
    kprintf("[TMPFS] ioctl only supports chardevs for now\n");
    for (;;)
      ;
  }

  TmpNode *tnode = vp->private_data;
  CharacterDevice chardev = tnode->dev.cdev;
  if (chardev.fs->ioctl)
    return chardev.fs->ioctl(vp, req, data, fflag);

  return -1;
}

int tmpfs_poll(VFSNode *vp, int events) {
  if (vp->stat.type != VFS_CHARDEVICE) {
    kprintf("[TMPFS] poll only supports chardevs for now\n");
    for (;;)
      ;
  }

  TmpNode *tnode = vp->private_data;
  CharacterDevice chardev = tnode->dev.cdev;
  if (chardev.fs->poll)
    return chardev.fs->poll(vp, events);

  return -1;
}

void tmpfs_dump(TmpNode *root) {
  if (TAILQ_EMPTY(&root->dir.dirents))
    return;

  struct tmpfs_dirent *dirent;
  TAILQ_FOREACH(dirent, &root->dir.dirents, entries) {
    if (dirent->inode->attr.type == VFS_DIRECTORY) {
      kprintf("%s (DIR)\n", dirent->filename);
      tmpfs_dump(dirent->inode);
    } else if (dirent->inode->attr.type == VFS_FILE) {
      kprintf("%s (FILE)\n", dirent->filename);
    }
  }
}

static int tmpfs_symlink(struct vnode *dvp, struct vnode **vpp, char *source,
                         struct vattr *vap, char *target) {

  kprintf("Creating symlink from %s to %s\n", source, target);

  vap->type = VFS_SYMLINK;
  TmpNode *new = tmakenode(dvp->private_data, strdup(source), vap);

  // set link
  new->link = target;

  return dvp->vfs->ops->vget(dvp->vfs, vpp, (ino_t) new);
}

static ssize_t tmpfs_readlink(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  return -1;
}

VNodeOps tmpfs_vnops = {.create = tmpfs_create,
                        .open = tmpfs_open,
                        .read = tmpfs_read,
                        .write = tmpfs_write,
                        .mkdir = tmpfs_mkdir,
                        .symlink = tmpfs_symlink,
                        .readdir = tmpfs_readdir,
                        .lookup = tmpfs_lookup,
                        .ioctl = tmpfs_ioctl,
                        .poll = tmpfs_poll};

int tmpfs_init() {
  vfs_root.ops = &tmpfs_vfsops;
  return vfs_root.ops->mount(&vfs_root, NULL, NULL);
}
