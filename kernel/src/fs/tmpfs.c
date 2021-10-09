#include <stdarg.h>

#include "vfs.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../kprintf.h"

int ustar_write(int argc, ...);
int ustar_close(int argc, ...);


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

int ustar_read(int argc,  ...){
    va_list args_list;

    va_start(args_list, argc);

    /*
     * Arguments: u8 * archive, const char * path, u8 * buffer
     */
    u8 * archive = va_arg(args_list, u8 *);
    const char * path = va_arg(args_list, const char *);
    u8 * buffer = va_arg(args_list, u8 *);

    u8 ** data_ptr;
    u64 filesize = tar_lookup(archive, path, data_ptr);

#ifdef TMPFS_DEBUG
    if(filesize == 0)
        kprintf("%s doesn't exist\n", path);

    kprintf("Read file %s; Contents:\n", path);
    kprintf("%s\n", *data_ptr);
#endif

    memcpy(buffer, *data_ptr, filesize);

    va_end(args_list);

    return 1;
}

Mount * init_tmpfs(u8 * tmpfs){
    Mount * ustar_mount = kmalloc(sizeof(Mount));

    ustar_mount->read = &ustar_read;

    /*
     * ustar_mount->write = &ustar_write;
     * ustar_mount->open =  &ustar_open;  
     * ustar_mount->close = &ustar_close;
     */
    
    return ustar_mount;
}

