#include "libk/util.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "misc/ssfn.h"
#include "proc/proc.h"
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <string/string.h>

#include <sys/queue.h>

#define TMPFS_VIRT_BASE 0xD000000000

static int tmpfs_mount(VFS *vfs, const char *path, void *data) {
  TmpNode *tmp_root_node = kmalloc(sizeof(TmpNode));
  VFSNode *tmp_root_vnode;

  TAILQ_INIT(&tmp_root_node->dir.dirents);

  tmp_root_node->attr.type = VFS_DIRECTORY;
  tmp_root_node->vnode = NULL;

  tmpfs_vfsops.vget(vfs, &tmp_root_vnode, (ino_t)tmp_root_node);
  tmp_root_vnode->isroot = true;
  vfs->private_data = tmp_root_node;
  tmp_root_node->vnode = tmp_root_vnode;

  root_vnode = tmp_root_vnode;

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

  VFSNode *vnode = kmalloc(sizeof(VFSNode));
  node->vnode = vnode;
  vnode->refcnt = 1;

  vnode->v_type = node->attr.type;
  vnode->ops = &tmpfs_vnops;
  vnode->vfs = vfs;
  vnode->vfs_mountedhere = NULL;
  vnode->isroot = false;

  vnode->private_data = node;
  *out = vnode;
  return 0;
}

VFSOps tmpfs_vfsops = {
    .mount = tmpfs_mount, .root = tmpfs_root, .vget = tmpfs_vget};

static int tmpfs_lookup(VFSNode *dvn, VFSNode **out,
                        struct componentname *cnp) {

  kprintf("Looking up %s\n", cnp->cn_nameptr);
  TmpNode *tnode = dvn->private_data;

  if (strcmp(cnp->cn_nameptr, "/") == 0) {
    VFSNode *ret;
    tmpfs_root(&vfs_root, &ret);
    kprintf("Found root at 0x%x\n", ret);
    *out = ret;
    return 0;
  }

  if (strcmp(cnp->cn_nameptr, ".") == 0) {
    dvn->refcnt++;
    *out = dvn;
    return 0;
  }

  if (strcmp(cnp->cn_nameptr, "..") == 0) {
    if (tnode->dir.parent) {
      *out = tnode->dir.parent->vnode;
      return 0;
    }
  }

  struct tmp_dir_q *new_dir = kmalloc(sizeof(struct tmp_dir_q));
  TAILQ_INIT(new_dir);

  struct tmpfs_dirent *dirent;
loop:
  TAILQ_FOREACH(dirent, &tnode->dir.dirents, entries) {
    if (strcmp(dirent->filename, cnp->cn_nameptr) == 0) {
      kprintf("Found entry: %s\n", dirent->filename);
      TmpNode *next_tnode = dirent->inode;
      *out = next_tnode->vnode;
      return 0;
    }

    // kprintf("Checking if %s starts with %s \n", cnp->cn_nameptr,
    // dirent->filename);
    if (strncmp(dirent->filename, cnp->cn_nameptr, strlen(dirent->filename)) ==
        0) {
      // kprintf("  ... it does\n");
      tnode = dirent->inode;
      goto loop;
    }
    // kprintf("\n");
  }

  // kprintf("Couldn't find %s\n", cnp->cn_nameptr);
  return -1;
}

TmpNode *tmakenode(TmpNode *dtn, const char *name, struct vattr *vap) {

  TmpNode *tnode = kmalloc(sizeof(TmpNode));
  struct tmpfs_dirent *dent = kmalloc(sizeof(struct tmpfs_dirent));

  dent->filename = kmalloc(strlen(name));
  strcpy(dent->filename, name);

  dent->inode = tnode;

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
    break;
  }
  case VFS_FILE: {
    TAILQ_INIT(&tnode->file.anonmap);
    break;
  }
  default:
    for (;;)
      kprintf("tmpfs::mknod() unhandled type\n");
  }

  TAILQ_INSERT_TAIL(&dtn->dir.dirents, dent, entries);

  return tnode;
}

static int tmpfs_create(VFSNode *dvn, VFSNode **out, ComponentName *cnp,
                        VAttr *vap) {

  TmpNode *new = tmakenode(dvn->private_data, cnp->cn_nameptr, vap);

  if (!new)
    kprintf("Couldn't make node :(\n");

  return dvn->vfs->ops->vget(dvn->vfs, out, (ino_t) new);
}

static int tmpfs_getattr(VFSNode *dvn, VAttr *out) { return -1; }
static int tmpfs_open(VFSNode *vn, VFSNode **out, int mode) { return -1; }

static int tmpfs_readdir(VFSNode *dvn, void *buf, size_t nbyte,
                         size_t *bytesRead, off_t seqno) {
  return -1;
}

int tmpfs_mkdir(VFSNode *dvp, VFSNode **out, ComponentName *cnp,
                struct vattr *vap) {
  kprintf("Called create on directory %s with size %d \n", cnp->cn_nameptr,
          vap->size);

  TmpNode *new = tmakenode(dvp->private_data, cnp->cn_nameptr, vap);
  if (!new)
    kprintf("Couldn't make node :(\n");

  return dvp->vfs->ops->vget(dvp->vfs, out, (ino_t) new);
}

static int tmpfs_read(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tnode = vn->private_data;
  if (nbyte + off > tnode->attr.size) {
    for (;;)
      kprintf("Reading outsie of file ... ");
  }

  void *start = TAILQ_FIRST(&tnode->file.anonmap)->page + off;
  memcpy(buf, start, nbyte);

  return 0;
}

int tmpfs_write(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tnode = vn->private_data;

  // kprintf("tnode is @ 0x%x\n", tnode);

  if (nbyte + off > tnode->attr.size) {
    int pages_needed =
        DIV_ROUND_UP((nbyte + off) - tnode->attr.size, PAGE_SIZE);
    void *pages = pmm_alloc_blocks(pages_needed);
    struct anon *na = kmalloc(sizeof(struct anon));
    na->page = pages;
    TAILQ_INSERT_TAIL(&tnode->file.anonmap, na, entries);

    tnode->attr.size = nbyte + off; // new size
  }

  void *start = TAILQ_FIRST(&tnode->file.anonmap)->page + off;
  memcpy(start, buf, nbyte);
  return 0;
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

VNodeOps tmpfs_vnops = {.create = tmpfs_create,
                        .open = tmpfs_open,
                        .read = tmpfs_read,
                        .write = tmpfs_write,
                        .mkdir = tmpfs_mkdir,
                        .readdir = tmpfs_readdir,
                        .lookup = tmpfs_lookup};
