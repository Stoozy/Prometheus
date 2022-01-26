#include "vfs.h"
#include "../kmalloc.h"

static FdCacheNode * gp_fd_cache;

VfsNode * vfs_node_from_path(const char * path, struct dirent cwd);
VfsNode * vfs_node_from_fd(int fd);


int vfs_open(VfsNode * node, int flags){

    // no such node
    if(!node)
        return -1;

    // TODO: search cache first
    int fd =  node->open(node); 

    FdCacheNode * fcn_node = (FdCacheNode*)kmalloc(sizeof(FdCacheNode));

    fcn_node->node = node;
    fcn_node->next = NULL;

    FdCacheNode * current_node = gp_fd_cache;

    if(current_node == NULL){
        current_node->node = node;
        current_node->next = NULL;
        return fd;
    }


    while(current_node->next != NULL)
        current_node = current_node->next;
    current_node->next = fcn_node;

    return fd;
}

int vfs_close(struct vnode * node){
    // TODO: write all changes to file here


    // remove from open file list
    FdCacheNode * current_node = gp_fd_cache;

    while(current_node->next != NULL ){
        // found fd
        if(current_node->next->node == node){
            FdCacheNode * fcn_ptr_to_remove = current_node->next;

            // relink
            current_node->next = current_node->next->next;

            kfree(fcn_ptr_to_remove);
            return 1;
        }
        current_node = current_node->next;
    }

    // non existent fd
    return -1;
    
}

int vfs_read(VfsNode * node, uint64_t offset, size_t size, uint8_t * buffer){
    if(!node)
        return -1; // non-existent node

    if(node->read)
        return node->read(node, offset, size, buffer);
    else return -1; // invalid node

}

int vfs_write(VfsNode * node, uint64_t offset, size_t size, uint8_t * buffer){
     if(!node)
        return -1; // non-existent node

    if(node->write)
        return node->write(node, offset, size, buffer);
    else return -1; // invalid node
}


void vfs_init(){
    
}


