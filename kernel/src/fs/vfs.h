#include "../typedefs.h"

#define VFS_FILE            0x01
#define VFS_DIRECTORY       0x02
#define VFS_CHARDEVICE      0x03
#define VFS_BLOCKDEVICE     0x04
#define VFS_PIPE            0x05
#define VFS_SYMLINK         0x06
#define VFS_MOUNTPOINT      0x08

struct node;


typedef struct {

    int (*read)     (int argc, ...);
    int (*write)    (int argc, ...);
    int (*open)     (int argc, ...);
    int (*close)    (int argc, ...);

} Mount;


typedef struct {
    char * name;
    u64 inode;
    u64 size;
    u64 position;
    u64 mode;       /* perms */
    u64 flags;

    Mount * fs;     /* fs specific functions */

} VfsNode;

VfsNode * vfs_node_from_path(const char * path, ...);
VfsNode * vfs_node_from_fd(int fd);

int     vfs_open(const char * path, int flags, ...);
int     vfs_close(int fd);
int     vfs_read();
int     vfs_write();

void vfs_set_root_mount(Mount *);
