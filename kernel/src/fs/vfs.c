#include "vfs.h"

static Mount * root;

VfsNode * vfs_node_from_path(const char * path, ...){
    // TODO

}

VfsNode * vfs_node_from_fd(int fd){
    // TODO
}

int  vfs_open(const char * path, int flags, ...){

    VfsNode * node = vfs_node_from_path(path);

    if(node->fs->open  != 0){
        return node->fs->open(1, path);
    }else{
        return 0;
    }
}

int  vfs_close(int fd){

    VfsNode * node = vfs_node_from_fd(fd);

    if(node->fs->close){
        return node->fs->close(0);
    }else{
        return 0;
    }
}

int vfs_read(VfsNode * node, char * buffer)
{
    if(node->fs->read != 0)
        return node->fs->read(node->inode, node->position, node->size, *buffer);
    else return 0;
}

int vfs_write(VfsNode * node, char * buffer)
{
    if(node->fs->write != 0)
        return node->fs->write(node->inode, node->position, node->size, *buffer);
    else return 0;
}

void vfs_set_root_mount(Mount * m){
    root = m;
}
