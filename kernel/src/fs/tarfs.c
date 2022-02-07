#include <stdarg.h>

#include "vfs.h"
#include "tarfs.h"
#include "../kmalloc.h"
#include "../string/string.h"
#include "../kprintf.h"
#include "../config.h"


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

static u32 ustar_decode_filesize(UstarFile*file){
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

static u64 round_to_512_bytes(u64 bytes){
    if(bytes % 512 == 0)
        return bytes;
    
    u64 padding = bytes % 512;

    return bytes + (512 - padding);
}



