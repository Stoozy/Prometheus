#include "vfs.h"
#include "../string/string.h"
#include "../kmalloc.h"
#include "../kprintf.h"

FileSystem * gp_filesystems;
VfsNode * gp_root;
VfsOpenListNode * gp_open_list;

static VfsNode * vfs_node_from_path(VfsNode * parent, const char * path){

    kprintf("[VFS]  Getting node from path %s\n", path);
    kprintf("[VFS]  Parent is %s\n", parent->file->name);

    size_t len = __builtin_strlen(path);


    File * file = parent->file->fs->finddir(parent, &path[1]);

    if(file){
        VfsNode * new_node = kmalloc(sizeof(VfsNode));
        new_node->file = file;
        new_node->parent = parent;
        new_node->children = NULL;
        new_node->next = NULL;
        new_node->type = VFS_FILE;

        return new_node;
    }


    /* TODO: 
     *
     * iterate the underlying fs
     * to see if file actually exists
     * otherwise, simply return null 
     */

    return NULL;
}


File * vfs_open(const char * filename , int flags){
    kprintf("[VFS]  Called open on %s\n", filename);
    VfsNode * node = vfs_node_from_path(gp_root, filename);

    if(node)
        return node->file;


    return NULL;

}

void vfs_close(File * file){
    VfsOpenListNode * current_node = gp_open_list;
    
    while(current_node->next != NULL){
        /* find the file */
        if(strcmp(current_node->next->vfs_node->file->name, file->name) == 0){
            VfsOpenListNode * free_this = current_node->next;

            /* relink here */
            VfsOpenListNode * new_next = current_node->next->next;
            current_node->next = new_next;

            kfree(free_this);
        }
    }

}

ssize_t vfs_read(File * file, u8 * buffer, size_t off, size_t size){
    kprintf("[VFS]  Called read on %s\n", file->name);
    if(file){
        size_t bytes = file->fs->read(file, size, buffer);
        file->position += size;
        return bytes;
    }

    return -1; 
}


ssize_t vfs_write(File * file,  u8 * buffer, size_t off, size_t size){

    if(file){
         file->position += off;
        size_t bytes = file->fs->read(file, size, buffer);
        return bytes;
    }

    return -1; 
}




void vfs_register_fs(FileSystem * fs, u64 device_id){
    if(gp_filesystems == NULL){
        gp_filesystems = fs;
    }else{
        FileSystem * cur_fs = gp_filesystems;

        while(cur_fs->next != NULL)
            cur_fs = cur_fs->next;
        cur_fs->next = fs;
    }

    return;
}

void vfs_unregister_fs(FileSystem * fs){
    /* TODO */


    return;
}


static FileSystem * fs_from_path(const char * path){
    FileSystem * fs;

    /* TODO */

    return fs;
}

bool vfs_mount(const char * src, const char * dst){
    /* trying to mount root */
    if(dst[0] == '/' && dst[1] == 0 && gp_root != NULL)
        return false;

    FileSystem * fs = fs_from_path(src);
    VfsNode * node = vfs_node_from_path(gp_root, dst);

    if(node){
        node->type = VFS_MOUNTPOINT;
        node->file = NULL;
    }else{
        /* trying to mount on non-existent dir */
        return false;
    }


    return true;
}

static void _vfs_rec_dump(VfsNode * node){
    kprintf("Found %s\n", node->file->name);

    if(node->type == VFS_DIRECTORY || node->type == VFS_MOUNTPOINT){
        kprintf("Is a directory. Iterating directory...\n");
        if(node->children)
            _vfs_rec_dump(node->children);
    }

    if(node->next)
        _vfs_rec_dump(node->next);
}

void vfs_dump(){
    _vfs_rec_dump(gp_root);
}


void vfs_init(FileSystem * root_fs){
    gp_root =  kmalloc(sizeof(VfsNode));

    gp_root->parent = NULL;
    gp_root->type = VFS_MOUNTPOINT;

    /* no need to iterate fs yet */
    gp_root->children =  NULL;

    gp_root->file = kmalloc(sizeof(File));
    gp_root->file->name = kmalloc(2);
    gp_root->file->name[0] = '/';
    gp_root->file->name[1] = '\0';
    
    gp_root->file->fs = root_fs;
    gp_root->file->device = root_fs->device;

    return;
}

