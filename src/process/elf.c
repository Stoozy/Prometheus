#include "elf.h"
#include "../string/string.h"

#define MIN2(a, b) ((a) < (b) ? (a) : (b))

i32 load_elf_bin(u8 * elf_image){
    ElfHeader64 header;
    ElfProgramHeader64 ph;

    memcpy(&header, elf_image, sizeof(header));

    /* invalid magic value */
    if(header.e_ident[0] != 0x7f || 
        header.e_ident[1] != 'E' ||
        header.e_ident[2] != 'L' ||
        header.e_ident[3] != 'F')

        return -1;

    
    return 0;

}

