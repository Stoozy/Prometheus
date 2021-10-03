#include "vfs.h"

int  vfs_open(VfsNode * node, const char * path){
    if(node->fs->open  != 0){
        return node->fs->open(1, path);
    }else{
        return 0;
    }
}

int  vfs_close(VfsNode * node, const char * path){
    if(node->fs->close  != 0){
        return node->fs->close(1, path);
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

