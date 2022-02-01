#include "vfs.h"
#include "../kmalloc.h"
#include "../kprintf.h"
#include "../string/string.h"
#include "../proc/proc.h"

static FdCacheNode * gp_fd_cache;
static FdCacheNode * gp_fd_cache_last;

static VfsNode * root;

VfsNode * vfs_node_from_path(const char * path){
    VfsNode * current_node = root;

    while(  strcmp(current_node->name, path) == 0
            && strlen(path) != strlen(current_node->name)){
        for(u64 i=0; i<current_node->num_children; ++i){
            if(strcmp(current_node->children[i]->name, path) == 0){
                current_node = current_node->children[i];
                // both length and content are same, found the
                // node
                if(strlen(current_node->name) == strlen(path)){
                    return current_node;
                }
            }
        }
    }

    return NULL;
}

VfsNode * vfs_node_from_fd(int fd);

int vfs_open(VfsNode * node, int flags){

    // no such node
    if(!node)
        return -1;

    // TODO: search cache first
    int fd =  node->open(node, flags); 

    extern ProcessControlBlock * gp_current_process;
    map_fd_to_proc(gp_current_process, node);

    return fd;
}

int vfs_close(struct vnode * node){
    // TODO: write all changes to file here

    // non existent fd
    return -1;
    
}

int vfs_read(VfsNode * node, uint64_t offset, size_t size, uint8_t * buffer){
    if(!node){
        kprintf("[VFS]  Trying to read nonexistent node\n");
        return -1; // non-existent node
    }

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
    VfsNode * root = kmalloc(sizeof(VfsNode));
    memset(root, 0, sizeof(VfsNode));
    root->name = "";
    root->type = VFS_MOUNTPOINT;
}


