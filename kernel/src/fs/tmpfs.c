#include "vfs.h"
#include "../kmalloc.h"
#include "../typedefs.h"

int tmpfs_open(int argc, ...);
int tmpfs_read(int argc, ...);
int tmpfs_write(int argc, ...);
int tmpfs_close(int argc, ...);




void init_tmpfs(u8 * tmpfs){
    Mount * tmpfs_mount = kmalloc(sizeof(Mount));

    tmpfs_mount->read = &tmpfs_read;
    tmpfs_mount->write = &tmpfs_write;
    tmpfs_mount->open = &tmpfs_open;
    tmpfs_mount->close = &tmpfs_close;
}

