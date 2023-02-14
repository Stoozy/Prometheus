#pragma once

#include "fs/vfs.h"

#include <stdint.h>
#include <sys/queue.h>
#include <unistd.h>

struct tmpnode;

struct tmpfs_dirent {
  TAILQ_ENTRY(tmpfs_dirent) entries;

  char *filename;
  struct tmpnode *inode;
};

typedef struct tmp_dir {
  TAILQ_HEAD(tmp_dir_q, tmpfs_dirent) dirents;
  struct tmpnode *parent;
} TmpDir;

struct anon {
  TAILQ_ENTRY(anon) entries;

  int swap;
  void *page;
};

typedef struct tmp_file {
  TAILQ_HEAD(, anon) anonmap;
} TmpFile;

typedef struct tmpnode {
  VFSNode *vnode;

  union {
    /* can be either a file or directory */
    TmpDir dir;
    TmpFile file;
  };

  VAttr attr;

} TmpNode;

int tmpfs_init();

extern struct vfsops tmpfs_vfsops;
extern struct vnops tmpfs_vnops;
