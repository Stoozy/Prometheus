#include <stdarg.h>

#include "vfs.h"
#include "tarfs.h"
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


u32 ustar_decode_filesize(UstarFile*file){
	return
		((file->size[ 0] - '0') << 30) |
		((file->size[ 1] - '0') << 27) |
		((file->size[ 2] - '0') << 24) |
		((file->size[ 3] - '0') << 21) |
		((file->size[ 4] - '0') << 18) |
		((file->size[ 5] - '0') << 15) |
		((file->size[ 6] - '0') << 12) |
		((file->size[ 7] - '0') <<  9) |
		((file->size[ 8] - '0') <<  6) |
		((file->size[ 9] - '0') <<  3) |
		((file->size[10] - '0') <<  0);
}



VfsNode * tarfs_init(u8 * archive){
    VfsNode * node = kmalloc(sizeof(VfsNode));

    // get the root sector
    UstarFile * root = (UstarFile *) (archive);
    kprintf("[TARFS]	Got root folder prefix %s\n", root->filename_prefix);
    kprintf("[TARFS]	Got root folder name %s\n", root->name);
    kprintf("[TARFS]	Got root size %lu\n", oct2bin(root->size, 11));
    kprintf("[TARFS]	Got root owner %s\n", root->owner_user_name);

    for(;;);

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

