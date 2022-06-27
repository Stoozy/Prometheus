#include <stdarg.h>

#include "vfs.h"
#include "tarfs.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../config.h"

struct file *   ustar_finddir(VfsNode * dir, const char * name);
struct dirent * ustar_readdir(VfsNode * dir, u32 index);
u64             ustar_write(struct file *file, u64 size, u8 *buffer);
u64             ustar_read(struct file *file, size_t size, u8 *buffer);
void            ustar_close(struct file * f);
struct file *   ustar_open(const char *filename, int flags);

FileSystem g_tarfs = {
    .name = "ustar\0",
    .device = 0,
    .open = ustar_open,
    .read = ustar_read,
    .close = ustar_close,
    .write = NULL,
    .readdir = ustar_readdir,
    .finddir = ustar_finddir
};


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

static u8 ustar_type_to_vfs_type(u8 type){
    switch(type){
        case '0':
            return VFS_FILE;
        case '1':
            return VFS_SYMLINK;
        case '2':
            return VFS_SYMLINK;
        case '3':
            return VFS_CHARDEVICE;
        case '4':
            return VFS_BLOCKDEVICE;
        case '5':
            return VFS_DIRECTORY;
        case '6':
            return VFS_PIPE;
        default:
            return VFS_INVALID_FS;
    }
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

    return 0;
}

static u64 round_to_512_bytes(u64 bytes){
    if(bytes % 512 == 0)
        return bytes;
    
    u64 padding = bytes % 512;

    return bytes + (512 - padding);
}


struct file * ustar_open(const char *filename, int flags){

    UstarFile * tar_file = ustar_search(g_archive, filename);

    if(tar_file){
        File  * file = kmalloc(sizeof(File));
        file->size = ustar_decode_filesize(tar_file);
        file->name = (char*)filename;
        file->inode = (u64)((void*)tar_file);
        file->fs = &g_tarfs;
        return file;
    }

    return 0;
}


void ustar_close(struct file * f){
    // TODO


}


u64 ustar_read(struct file *file, size_t size, u8 *buffer){
    // TODO:
    
    kprintf("[TARFS] Called read\n");

    UstarFile * tar_fp = (UstarFile*) file->inode;
    u8 * sof = ((u8*)(tar_fp))+512;
    kprintf("[TARFS] File data: %s\n", sof);

    u8 * begin = sof + file->position;
    
    if(tar_fp){
        kprintf("[TARFS] Valid file\n");
        kprintf("Copying data over to %x\n", buffer);

        memcpy(buffer, begin, size);

        return size;
    }

    return 0;
}

u64 ustar_write(struct file *file, u64 size, u8 *buffer){
    // TODO:
    // return bytes written
    

    return 0;
}


struct dirent * ustar_readdir(VfsNode * dir, u32 index){
    /* make sure node is actually a directory */
    if(dir->type != VFS_DIRECTORY)
        return NULL;

    struct dirent * ret = kmalloc(sizeof(struct dirent));

    /* TODO */

    return ret;
}


struct file * ustar_finddir(VfsNode * dir, const char * name){

    kprintf("[TARFS]    Finding %s in current directory %s\n", name, dir->file->name);

    UstarFile * file = ustar_search(g_archive, name);

    if(file){
        kprintf("[TARFS]    Found %s\n", file->name);
        struct file * vfs_file = kmalloc(sizeof(struct file));
        vfs_file->name = file->name;
        vfs_file->inode = (u64)((void*)file);
        vfs_file->size = ustar_decode_filesize(file);
        vfs_file->position = 0; 
        vfs_file->device = 0;  
        vfs_file->next = NULL;


        vfs_file->fs =  &g_tarfs;

        return vfs_file;
    }

    return NULL;
}

FileSystem * tarfs_init(u8 * archive){
    g_archive = archive;

    FileSystem * tarfs = &g_tarfs; 

    tarfs->open = &ustar_open;
    tarfs->close = &ustar_close;
    tarfs->read = &ustar_read;
    tarfs->readdir = &ustar_readdir;
    tarfs->finddir = &ustar_finddir;

    /* read only tmp fs for now */
    tarfs->write = NULL;
    /*tarfs->write = &ustar_write;*/

    tarfs->next = NULL;

    return tarfs;
}

