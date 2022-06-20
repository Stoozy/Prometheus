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
#define VFS_INVALID_FS      0x09

struct file;

typedef struct sector_node {
    u64 sector;
    struct sector_node * next;
} SectorNode;



typedef struct file {

    char * name;
    SectorNode * sectors;
    u64 device;
    u64 position;
    u64 size;

    u8  type;
    u8  mode;

} File;

typedef File * (*open_func_t)(const char * filename, int flags);
typedef void (*close_func_t)(struct file *);
typedef u64 (*read_func_t)(struct file *, u64, u8 *);
typedef u64 (*write_func_t)(struct file *, u64, u8 *);
typedef struct file * (*finddir_func_t)(const char * dirname);



typedef struct fs {

    char * name;

    u64 device;
    open_func_t     open;
    close_func_t    close;
    read_func_t     read;
    write_func_t    write;
    finddir_func_t  finddir; 

} FileSystem;


typedef struct vfs_node {
    

    FileSystem * fs;
    File * file;

    u64 num_children;
    struct vfs_node * children;

}   VfsNode;


typedef struct open_list_node {

    VfsNode * vfs_node;
    struct open_list_node * next;

} VfsOpenListNode;



File * vfs_open(const char * filename , int flags);
void vfs_close(File file);

u64 vfs_read(File * file, u64 size, u8 * buffer);
u64 vfs_write(File * file, u64 size, u8 * buffer);

void vfs_register_fs( const char * path ,FileSystem *, u64 device_id);
void vfs_unregister_fs(FileSystem * fs);

void vfs_init(FileSystem *);

