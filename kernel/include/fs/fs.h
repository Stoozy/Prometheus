#pragma once


#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

// simple name/inode pair
typedef struct dirent {
    char name[256];
    uint32_t inode;
} dirent_t;


typedef struct fs_node{
    uint32_t mode;          
    uint32_t uid;           
    uint32_t gid;           
    uint32_t flags;         
    uint32_t inode;         
    uint32_t length;        
    uint32_t impl;               
    
    /* File I/O functions */
    
    uint32_t (*read)            (struct fs_node *, uint32_t, uint32_t, uint8_t *);
    uint32_t (*write)           (struct fs_node *, uint32_t, uint32_t, uint8_t *);

    void (*open)                (struct fs_node *);
    void (*close)               (struct fs_node *);

    struct dirent  *  (*readdir)       (struct fs_node *, uint32_t);
    struct fs_node *  (*finddir)       (struct fs_node *, char * name);
    struct fs_node * ptr; // Used by mountpoints and symlinks.

} fs_node_t;


typedef struct file_system_type {
    char name[128];             // file system name e.g. ext2, fat, etc
    dirent_t * (*mount)         (struct file_system_type *, const char *);
    void (*umount)              (fs_node_t *);
}fs_type_t; 


int register_fs(fs_type_t * type);
int unregister_fs(fs_type_t * type);
    



