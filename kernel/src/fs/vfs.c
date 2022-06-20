#include "vfs.h"
#include "../string/string.h"
#include "../kmalloc.h"


VfsNode * gp_root;
VfsOpenListNode * gp_open_list;


static VfsNode * children_from_fs(FileSystem * fs, u64 * count){
    /* TODO */
    return NULL;
}

static VfsNode * vfs_node_from_path(const char * path){
    /* TODO */
    return NULL;
}

File * vfs_open(const char * filename , int flags){
    VfsNode * node = vfs_node_from_path(filename);

    if(node)
        return node->fs->open(filename, flags);

    return NULL;

}

void vfs_close(File file){
    VfsOpenListNode * current_node = gp_open_list;
    
    while(current_node->next != NULL){
        if(strcmp(current_node->next->vfs_node->file->name, file.name) == 0){
            VfsOpenListNode * free_this = current_node->next;

            /* relink here */
            VfsOpenListNode * new_next = current_node->next->next;
            current_node->next = new_next;

            kfree(free_this);
        }
    }

}

u64 vfs_read(File * file, u64 size, u8 * buffer){
    /* TODO */
    return 0;
}

u64 vfs_write(File * file, u64 size, u8 * buffer){
    /* TODO */
    return 0;
}


void vfs_register_fs(const char * path, FileSystem * fs, u64 device_id){
    VfsNode * node = vfs_node_from_path(path);

    memset(node, 0, sizeof(VfsNode));

    node->file->type = VFS_MOUNTPOINT;
    node->file->device = device_id;

    node->fs = fs;

    /* TODO: populate children based on actual fs */

    u64 num_children;

    node->children = children_from_fs(fs, &num_children);
    node->num_children = num_children;


    return;
}

void vfs_unregister_fs(FileSystem * fs){

    /* TODO */
    return;
}

void vfs_init(FileSystem * root_fs){
    gp_root = kmalloc(sizeof(VfsNode));
    gp_root->fs = root_fs;

    gp_root->file = kmalloc(sizeof(File));
    gp_root->file->type = VFS_MOUNTPOINT;
    gp_root->file->device = root_fs->device;

    u64 num_children;

    gp_root->children = children_from_fs(root_fs, &num_children);
    gp_root->num_children = num_children;

    return;
}


