#ifndef _ELF_H
#define _ELF_H  1

#include "../typedefs.h"

typedef struct {
    u32 magic;
    u8  bits;
    u8  endianness;
    u8  abi;
    u64 padding; 
    u16 type;
    u16 instruction_set;
    u32 elf_version;
    u64 p_entry_position;
    u64 p_header_table_position;
    u64 s_header_table_position;
    u32 flags;
    u16 header_size;
    u16 p_header_entry_size;
    u16 p_header_entries;
    u16 s_header_entry_size;
    u16 s_header_entries;
    u16 s_header_index;
} ElfHeader64;


#endif