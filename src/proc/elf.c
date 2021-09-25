#include "elf.h"
#include "../typedefs.h"


u8 load_elf_64(u8 * elf);
u8 load_elf_32(u8 * elf);

u8 load_elf_bin(u8 * elf) {
    if(elf[0] != 0x7f 
        || elf[1] != 'E'
        || elf[2] != 'L'
        || elf[3] != 'F')
    {
        /* invalid elf header */
        return -1;
    }

    if(elf[4] == 1){
        /* 32 bit elf file */
        return load_elf_32(elf);
    }else{
        return load_elf_64(elf);
    }
}

u8 load_elf_64(u8 * elf){
    Elf64_Ehdr * elf64 = (Elf64_Ehdr *) elf;
    return 0;
}

u8 load_elf_32(u8 * elf){
    Elf32_Ehdr * elf32 = (Elf32_Ehdr*)elf;
    return 0;
}
