#pragma once

#include "fs/vfs.h"

struct anon {
    int swap;
    void * page;
};

struct tmpnode;

struct tmpfs_dirent {
    char * filename;
    struct tmpnode * inode;
    struct tmpfs_dirent * next;
};


struct tmpnode {
    VFSNode * vnode;

    struct tmpfs_dirent * dirents;

    int status;
    uint64_t device;
    uint64_t position;
    size_t size;
    uint8_t  mode;
    uint8_t  type;
};

int tmpfs_init();

extern struct vfsops tmpfs_vfsops;
extern struct vnops tmpfs_vnops;
