#pragma once

#include <typedefs.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>


#define VFS_FILE            0x01
#define VFS_DIRECTORY       0x02
#define VFS_CHARDEVICE      0x03
#define VFS_BLOCKDEVICE     0x04
#define VFS_PIPE            0x05
#define VFS_SYMLINK         0x06
#define VFS_MOUNTPOINT      0x08
#define VFS_INVALID_FS      0x09


#define NAME_MAX			128


struct dirent {
	char name[NAME_MAX];
	u32 ino;
}; 

struct file;
struct vfs_node;

typedef struct file * (*open_func_t)(const char * filename, int flags);
typedef void (*close_func_t)(struct file *);
typedef u64 (*read_func_t)(struct file *, size_t count, u8 * buf);
typedef u64 (*write_func_t)(struct file *, size_t count, u8 * buf);
typedef struct dirent * (*readdir_func_t)(struct vfs_node *, u32 index);
typedef struct file * (*finddir_func_t)(struct vfs_node *, const char * filename);


typedef struct fs {

    char * name;

    u64 device;

    open_func_t     open;
    close_func_t    close;
    read_func_t     read;
    write_func_t    write;
    readdir_func_t  readdir;
    finddir_func_t  finddir; 

    struct fs * next;

} FileSystem;


typedef struct file {

    char * name;
    u64 inode;
    u64 device;
    u64 position;
    u64 size;
    u8  mode;


    FileSystem * fs;

    FileSystem * next;

} File;

typedef struct vfs_node {

    File * file;

    u8  type;

    struct vfs_node * parent;
    struct vfs_node * children;     /* points to the head of children */

    struct vfs_node * next;         /* next node in current directory */
}   VfsNode;


typedef struct open_list_node {

    VfsNode * vfs_node;
    struct open_list_node * next;

} VfsOpenListNode;


void vfs_dump();

bool vfs_mount(const char * src, const char * dst);

File * vfs_open(const char * filename , int flags);
void vfs_close(File * file);

ssize_t vfs_read(File * node, u8 * buffer, size_t off, size_t size);
ssize_t vfs_write(File * node,  u8 * buffer, size_t off, size_t size);

void vfs_register_fs(FileSystem *, u64 device_id);
void vfs_unregister_fs(FileSystem * fs);

void vfs_init(FileSystem *);

