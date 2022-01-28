#include <stdarg.h>

#include "vfs.h"
#include "tarfs.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../config.h"





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

u32 ustar_decode_filesize(UstarFile*file){
    return oct2bin(file->size, 11);
}


UstarFile * ustar_search(unsigned char * archive, const char * filename){
    unsigned char *ptr = archive;

    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);

        if (!memcmp(ptr, filename, strlen(filename) + 1)) {
            UstarFile * file = (UstarFile*)ptr;
            return file;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }

    return NULL;
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

u64 round_to_512_bytes(u64 bytes){
    if(bytes % 512 == 0)
        return bytes;
    
    u64 padding = bytes % 512;

    return bytes + (512 - padding);
}



int ustar_read(struct vnode * node, u64 offset, u64 size, u8 * buffer){
    // TODO
    
    UstarFile * file = ustar_search(node->device, node->name);
    if(!file)
        return -1;
    u64 filesize = ustar_decode_filesize(file);

    u8 * file_start = (u8*)file+512;
    u8 * begin = file_start+offset;

    for(u8 * byte=begin; byte < begin+size; ++byte){
        *buffer += *byte;
        buffer++;
    }

    return 1;
    
}


int ustar_write(struct vnode * node, u64 offset, u64 size, u8 * buffer){
    // TODO

    return -1;
}

int ustar_open(struct vnode * node, int flags){
    // TODO

    return -1;
}


int ustar_close(struct vnode * node, int flags){
    // TODO:
    return -1;
}


VfsNode * tarfs_init(u8 * archive){
    VfsNode * node = kmalloc(sizeof(VfsNode));

    // get the root sector
    //UstarFile * root = (UstarFile *) (archive);
    //kprintf("[TARFS]	Got root folder prefix %s\n", root->filename_prefix);
    //kprintf("[TARFS]	Got root folder name %s\n", root->name);
    //kprintf("[TARFS]	Got root size %lu\n", oct2bin(root->size, 11));
    //kprintf("[TARFS]	Got root owner %s\n", root->owner_user_name);
    //

    //UstarFile * file = ustar_search(archive, "tmpfs/testfile");
    //kprintf("[TARFS]    Found file %s with size %llu\n", file->name, ustar_decode_filesize(file));

    //for(;;);

    node->inode = 0;
    node->name = "/";
    node->flags = VFS_DIRECTORY;
    node->size = 0x1000;
    node->read = &ustar_read;
    node->open =  &ustar_open;
    node->write = &ustar_write;
    node->close = &ustar_close;
    node->device = (void*)archive;

    return node;
}

