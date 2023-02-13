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

    kprintf("Checking if %s starts with %s \n", cnp->cn_nameptr,
            dirent->filename);
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

static int tmpfs_create(VFSNode *dvn, VFSNode **out, ComponentName *cnp,
                        VAttr *vap) {
  TmpNode *tmp_dir_node = dvn->private_data;

  TmpNode *new_tnode = kmalloc(sizeof(TmpNode));
  VFSNode *new_vnode = kmalloc(sizeof(VFSNode));

  struct tmpfs_dirent *new_dirent = kmalloc(sizeof(struct tmpfs_dirent));

  new_tnode->attr.type = VFS_FILE;
  new_tnode->attr.size = vap->size;
  kprintf("set size to %d \n", new_tnode->attr.size);

  new_dirent->inode = new_tnode;
  new_dirent->filename = cnp->cn_nameptr;
  new_tnode->dir.parent = tmp_dir_node;

  new_tnode->vnode = new_vnode;
  new_vnode->private_data = new_tnode;

  TAILQ_INSERT_TAIL(&tmp_dir_node->dir.dirents, new_dirent, entries);
  *out = new_vnode;
  return 0;
}

static int tmpfs_getattr(VFSNode *dvn, VAttr *out) { return -1; }
static int tmpfs_open(VFSNode *vn, VFSNode **out, int mode) { return -1; }

static int tmpfs_readdir(VFSNode *dvn, void *buf, size_t nbyte,
                         size_t *bytesRead, off_t seqno) {
  return -1;
}

int tmpfs_mkdir(VFSNode *dvp, VFSNode **vpp, ComponentName *cnp,
                struct vattr *vap) {
  kprintf("Called create on directory %s with size %d \n", cnp->cn_nameptr,
          vap->size);

  TmpNode *tmp_dir_node = dvp->private_data;
  TmpNode *new_tnode = kmalloc(sizeof(TmpNode));
  VFSNode *new_vnode = kmalloc(sizeof(VFSNode));

  struct tmpfs_dirent *new_dirent = kmalloc(sizeof(struct tmpfs_dirent));

  TAILQ_INIT(&new_tnode->dir.dirents);

  new_tnode->attr.type = VFS_DIRECTORY;
  new_dirent->inode = new_tnode;

  new_dirent->filename = cnp->cn_nameptr;
  new_tnode->dir.parent = tmp_dir_node;

  new_tnode->vnode = new_vnode;
  new_vnode->private_data = new_tnode;

  TAILQ_INSERT_TAIL(&tmp_dir_node->dir.dirents, new_dirent, entries);

  kprintf("Created %s at %x\n", new_dirent->filename, new_tnode);
  *vpp = new_vnode;
  return 0;
}

static int tmpfs_read(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  TmpNode *tmpnode = vn->private_data;

  struct anon *first = TAILQ_FIRST(&tmpnode->file.anonmap);
  void *startpg = first->page + nbyte + off;

  if (nbyte + off > tmpnode->attr.size) {
    kprintf("Reading outside of file ... \n");
    return -1;
  }

  memcpy(buf, startpg + off, nbyte);
  return 0;
}

int tmpfs_write(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  kprintf("Called tmpfs_write, buffer at %x; size %d\n", buf, nbyte);

  TmpNode *tmpnode = (TmpNode *)vn->private_data;
  kprintf("File size is %d\n", tmpnode->attr.size);

  if (nbyte + off > tmpnode->attr.size) {
    kprintf("Extending file isn't supported yet\n");
    return -1;
  }

  struct anon *first = TAILQ_FIRST(&tmpnode->file.anonmap);
  void *startpg = first->page + nbyte + off;

  memcpy(startpg + off, buf, nbyte);

  return 0;
}

void tmpfs_dump(TmpNode *root) {
  if (TAILQ_EMPTY(&root->dir.dirents))
    return;

  struct tmpfs_dirent *dirent;
  TAILQ_FOREACH(dirent, &root->dir.dirents, entries) {
    kprintf("%s \n", dirent->filename);
    tmpfs_dump(dirent->inode);
  }
}

VNodeOps tmpfs_vnops = {.create = tmpfs_create,
                        .open = tmpfs_open,
                        .read = tmpfs_read,
                        .write = tmpfs_write,
                        .mkdir = tmpfs_mkdir,
                        .readdir = tmpfs_readdir,
                        .lookup = tmpfs_lookup};
