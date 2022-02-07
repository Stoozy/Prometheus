#pragma once

#include "../typedefs.h"
#include "vfs.h"

#define USTAR_TYPE_NORMAL       0x0
#define USTAR_TYPE_HARDLINK     0x1
#define USTAR_TYPE_SYMLINK      0x2
#define USTAR_TYPE_CHARDEV      0x3
#define USTAR_TYPE_BLOCKDEV     0x4
#define USTAR_TYPE_DIRECTORY    0x5
#define USTAR_TYPE_FIFO         0x6

typedef struct {
    char    name[100];
    u64     mode;
    u64     owner_id;
    u64     group_id;
    u8      size[12];
    u8      last_modified[12];
    u64     checksum;  
    u8      type;
    u8      linked_file_name[100];
    u8      indicator;
    u8      version[2];
    u8      owner_user_name[32];
    u8      owner_group_name[32];
    u64     dev_major_number;
    u64     dev_minor_number;
    u8      filename_prefix[155];
} __attribute__((packed)) UstarFile;


typedef struct {
    UstarFile * file_header;
    int inode;

    u64 data_offset;
    size_t filesize;
} __attribute__((packed)) UstarEntry;

