#pragma once

#include "../typedefs.h"
#include <stddef.h>
#include <stdint.h>

#define VFS_FILE            0x01
#define VFS_DIRECTORY       0x02
#define VFS_CHARDEVICE      0x03
#define VFS_BLOCKDEVICE     0x04
#define VFS_PIPE            0x05
#define VFS_SYMLINK         0x06
#define VFS_MOUNTPOINT      0x08

struct vnode;

typedef int (*read_func_t)(struct vnode *, u64, u64, u8 *);
typedef int (*write_func_t)(struct vnode *, u64, u64, u8 *);
typedef int (*open_func_t)(struct vnode *, int);
typedef int (*close_func_t)(struct vnode *, int);

typedef struct dirent (*readdir_func_t)(struct vnode *, u64);
typedef struct vnode * (*finddir_func_t)(struct vnode *, char *);

struct dirent {
    uint64_t d_ino; /* File serial number */
    char * d_name;
};


typedef struct vnode {
    char * name;
    u64 inode;
    u64 size;
    u64 position;
    u64 mode;       
    u64 flags;
    
    read_func_t     read;
    write_func_t    write;
    open_func_t     open;
    close_func_t    close;

    struct dirent   readdir;
    struct vnode *  finddir;

    void * device;

    struct vnode ** children; // array of children
    uint64_t num_children;

} VfsNode;

typedef struct fd_cache{
    VfsNode * node;
    struct fd_cache * next;
} FdCacheNode;



VfsNode * vfs_node_from_path(const char * path, struct dirent cwd); 
VfsNode * vfs_node_from_fd(int fd);

int vfs_open(VfsNode * node, int flags);
int vfs_close(VfsNode * node);
int vfs_read(VfsNode * node, uint64_t offset, size_t size, uint8_t * buffer);
int vfs_write(VfsNode * node, uint64_t offset, size_t size, uint8_t * buffer);
void vfs_init();
