#include <stdarg.h>

#include "vfs.h"
#include "tarfs.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../config.h"


static u8 * g_archive;

static u32 oct2bin(unsigned char *str, int size) {
    int n = 0;
    unsigned char *c = str;
    while (size-- > 0) {
        n *= 8;
        n += *c - '0';
        c++;
    }
    return n;
}

static u32 ustar_decode_filesize(UstarFile * file){
    return oct2bin(file->size, 11);
}



UstarFile * ustar_search(unsigned char * archive, const char * filename){
    unsigned char *ptr = archive;

    while (!memcmp(ptr + 257, "ustar", 5)) {
        int filesize = oct2bin(ptr + 0x7c, 11);

        kprintf("[TARFS]    Discovered %s\n", ptr);
        if (!memcmp(ptr, filename, strlen(filename) + 1)) {
            UstarFile * file = (UstarFile*)ptr;
            return file;
        }
        ptr += (((filesize + 511) / 512) + 1) * 512;
    }

    return 0;
}

static u64 round_to_512_bytes(u64 bytes){
    if(bytes % 512 == 0)
        return bytes;
    
    u64 padding = bytes % 512;

    return bytes + (512 - padding);
}

FILE * ustar_open(const char *filename, int flags){

    UstarFile * tar_file = ustar_search(g_archive, filename);

    if(tar_file){
        FILE  * f = kmalloc(sizeof(FILE));
        f->size = ustar_decode_filesize(tar_file);
        f->name = (char*)filename;
        f->inode = (u64)((void*)tar_file);
        return f;
    }

    return 0;
}

void ustar_close(struct file * f){
    // TODO


}

u64 ustar_read(struct file *file, u64 size, u8 *buffer){
    // TODO:
    // return bytes read
    
    kprintf("[TARFS] Called read\n");

    UstarFile * tar_fp = (UstarFile*) file->inode;
    u8 * sof = ((u8*)(tar_fp))+512;
    kprintf("[TARFS] File data: %s\n", sof);
    if(tar_fp){
        kprintf("[TARFS] Valid file\n");
        memcpy(buffer, sof, size);
        return size;
    }

    return 0;
}

u64 ustar_write(struct file *file, u64 size, u8 *buffer){
    // TODO:
    // return bytes written
    

    return 0;
}


struct file ustar_finddir(const char * dirname){
    // TODO
    FILE  * f = kmalloc(sizeof(FILE));


    return *f;
}

FileSystem * tarfs_init(u8 * archive){
    g_archive = archive;

    FileSystem * tarfs = kmalloc(sizeof(FileSystem)); 

    tarfs->open = &ustar_open;
    tarfs->close = &ustar_close;
    tarfs->read = &ustar_read;
    tarfs->write = &ustar_write;
    tarfs->finddir = &ustar_finddir;

    return tarfs;
}


