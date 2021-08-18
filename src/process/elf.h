#ifndef _ELF_H
#define _ELF_H  1

#include "../typedefs.h"

#define EI_NIDENT   16

typedef struct {
    u8  e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine; 
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsz; 
    u16 e_shnum;
    u16 e_shstridx;
} ElfHeader64;

typedef struct {
    u32 p_type;
    u32 flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 undefined;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} ElfProgramHeader64;

#endif