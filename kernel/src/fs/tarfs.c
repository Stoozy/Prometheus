#include <stdarg.h>

#include "vfs.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../config.h"


// emulate inodes 
// 
// list with inode->filename maps
struct nodeMap  {
    int inode;
    char md[512]; // metadata
    struct nodeMap * next;
};


u8 * tar;

u32 oct2bin(unsigned char *str, int size) {
    int n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

/* returns file size and pointer to file data in out */
u64 tar_lookup(unsigned char *archive, const char *filename, unsigned char **out) {
    unsigned char *ptr = archive;

    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);

        if (!memcmp(ptr, filename, strlen(filename) + 1)) {
            *out = ptr + 512;
            return filesize;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }
    return 0;
}

int ustar_read(struct vnode * node, u64 offset, u64 size, u8 * buffer){

    // TODO

    return -1;
}


int ustar_write(struct vnode * node, u64 offset, u64 size, u8 * buffer){
    // TODO
    return -1;
}

int ustar_open(struct vnode * node){
    // TODO:
    return -1;
}


int ustar_close(struct vnode * node){
    // TODO:
    return -1;
}



VfsNode * tmpfs_init(u8 * archive){
    VfsNode * node = kmalloc(sizeof(VfsNode));
    node->inode = 0;
    node->name = "/";
    node->flags = VFS_DIRECTORY;
    node->size = 0x1000;
    node->read = &ustar_read;
    node->open =  &ustar_open;
    node->write = &ustar_write;
    node->close = &ustar_close;

    // TODO:
    // Traverse the actual fs,
    // return root node with all
    // children
   
    return node;
}

