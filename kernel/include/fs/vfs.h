#pragma once

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


#define name_max            128

typedef struct  {
    char name[name_max];
    uint32_t ino;
    uint32_t type;
} __attribute__((packed)) DirectoryEntry; 



struct file;
struct vfs_node;

typedef struct file * (*open_func_t)(const char * filename, int flags);
typedef void (*close_func_t)(struct file *);
typedef uint64_t (*read_func_t)(struct file *, size_t count, uint8_t * buf);
typedef uint64_t (*write_func_t)(struct file *, size_t count, uint8_t * buf);
typedef DirectoryEntry * (*readdir_func_t)(struct vfs_node *, uint32_t index);
typedef struct file * (*finddir_func_t)(struct vfs_node *, const char * filename);


typedef struct fs {

    char * name;

    uint64_t device;

    open_func_t     open;
    close_func_t    close;
    read_func_t     read;
    write_func_t    write;
    readdir_func_t  readdir;
    finddir_func_t  finddir; 

    struct fs * next;

} FileSystem;



typedef struct vfs_node_stat  {
    uint32_t type;
    uint64_t inode;
    uint64_t filesize;
} __attribute__((packed)) VfsNodeStat;

typedef struct file {

    char * name;
    int status;
    uint64_t inode;
    uint64_t device;
    uint64_t position;
    uint64_t size;

    uint8_t  mode;

    uint8_t  type;

    FileSystem  * fs;

    FileSystem  * next;

} File;

typedef struct vfs_node {

    File * file;


    struct vfs_node * parent;
    struct vfs_node * children;     /* points to the head of children */

    struct vfs_node * next;         /* next node in current directory */
}   VfsNode;


typedef struct mountpoint {
    char * path;
    FileSystem * fs;
    struct mountpoint * next;
} Mountpoint;


typedef struct open_list_node {

    VfsNode * vfs_node;
    struct open_list_node * next;

} VfsOpenListNode;


void vfs_dump();

DirectoryEntry * vfs_readdir(File * file);

File * vfs_open(const char * filename , int flags);
void vfs_close(File * file);

ssize_t vfs_read(File * file, uint8_t * buffer, size_t size);
ssize_t vfs_write(File * file,  uint8_t * buffer, size_t size);

void vfs_get_stat(const char * path, VfsNodeStat * res);

void vfs_register_fs(FileSystem *, uint64_t device_id);
void vfs_unregister_fs(FileSystem * fs);
int vfs_mount(const char * src, const char * dst, const char * fs_type, uint64_t flags, const char * data);

void vfs_init();

