#pragma once
#include <kernel/typedefs.h>


/*
 * Inode mode types 
 */
#define EXT2_MAX_FILENAME_LEN           256 // http://www.science.smith.edu/~nhowe/teaching/csc262/oldlabs/ext2.html
#define EXT2_FIFO                       0x1000 	
#define EXT2_CHARDEV                    0x2000
#define EXT2_DIRENT                     0x4000
#define EXT2_BLOCKDEV                   0x6000
#define EXT2_FILE                       0x8000
#define EXT2_SYMBOLIC_FILE              0xA000
#define EXT2_UNIX_SOCKET                0xC000

/* 
 * Dirent types
 */
#define EXT2_DIRENT_UNKNOWN                 0
#define EXT2_DIRENT_FILE                    1
#define EXT2_DIRENT_DIR                     2
#define EXT2_DIRENT_CHARDEV                 3
#define EXT2_DIRENT_BLOCKDEV                4
#define EXT2_DIRENT_FIFO                    5
#define EXT2_DIRENT_SOCKET                  6
#define EXT2_SYMBOLIC_LINK                  7

#define FS_ERR_STATE                        1
#define FS_CLEAN_STATE                      2
#define SECTOR_SIZE                         512
#define BGDT_SIZE                           32
#define BGDT_PER_SECTOR                     16
#define ROOT_INODE                          2

typedef struct EXT2FS_SUPERBLOCK{

    uint32_t n_inodes;
    uint32_t n_blocks;
    uint32_t n_res_blocks; /* Number of blocks reserved for superuser */
    uint32_t n_unalloc_blocks; 
    uint32_t n_unalloc_inodes;
    uint32_t superblock_block; /* block number of the block containing the superblock */

    uint32_t block_size; /* shift 1024 by this many bits to get the actual block size */
    uint32_t frag_size; /*  same as above */

    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;

    uint32_t last_mount_time; /* POSIX time */
    uint32_t last_written_time;  /* "   " */

    uint16_t n_mounts_since_fsck;
    uint16_t n_mounts_until_fsck;

    uint16_t signature;
    uint16_t fs_state;
    uint16_t err_action;
    uint16_t v_minor;

    uint32_t last_fsck;  /* POSIX time */
    uint32_t forced_fsck_interval;  /* POSIX time */
    uint32_t creation_osid;  
    uint32_t v_major;  

    uint16_t res_blocks_uid;  
    uint16_t res_blocks_gid;  

    /*
     * Starting extended superblock
     * fields. These are only present on 
     * major versions >= 1
     */
    
    uint32_t first_non_res_inode;

    uint16_t inode_size;
    uint16_t superblock_block_group;

    uint32_t optional_features;

    /* Features that if not supported, 
     * the volume must be mounted read-only 
     * 
     */
    uint32_t req_present_features; 
    uint32_t req_features;

    uint8_t fs_id[16];
    uint8_t volume_name[16];
    uint8_t last_mount_path[64];

    uint32_t comp_algo;

    uint8_t n_blocks_file_prealloc;
    uint8_t n_blocks_dir_prealloc;
    uint16_t padding_2;

    uint8_t journal_id[16];

    uint32_t journal_inode;
    uint32_t journal_dev;
    uint32_t orphan_inode_list_head;

    uint8_t padding_3[788];

} e2_superblock_t;



typedef struct EXT2FS_BLOCK_GROUP_DESC {

    uint32_t block_usage_bitmap_addr; /* block address of block usage bitmap */
    uint32_t inode_usage_bitmap_addr;
    uint32_t inode_table_addr;
    uint16_t n_unalloc_blocks;
    uint16_t n_unalloc_inodes;
    uint16_t n_dirs;

    uint8_t pad[18];

} e2_bgdt_t;


typedef struct EXT2FS_INODE{
    uint16_t mode;
    uint16_t user_id;
    uint32_t size;

    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_mod_time;
    uint32_t deletion_time;

    uint16_t group_id;

    uint16_t n_hardlinks;
    uint32_t n_sectors; /* Number of disk sectors in use by this node */

    uint32_t flags;
    uint32_t os_value_1;
    uint32_t direct_block_ptrs[12];


    uint32_t singly_indirect_block_ptr;
    uint32_t doubly_indirect_block_ptr;
    uint32_t triply_indirect_block_ptr;

    uint32_t gen_number; /* used by NFS */

    uint32_t padding_1;
    uint32_t padding_2;

    uint32_t frag_block_address;

    uint8_t os_value_2[12];

} e2_inode_t;


typedef struct EXT2FS_DIR_ENTRY{

    uint32_t    inode; 
    uint16_t    total_size;
    uint8_t     name_len;
    uint8_t     type;
    uint8_t     name_chars[EXT2_MAX_FILENAME_LEN];

} e2_dirent_t;

void init_fs(uint32_t * superblock, uint8_t drive);
void fs_dump_info(uint8_t drive);

uint32_t get_block_size(e2_superblock_t * superblock);
uint32_t get_frag_size(e2_superblock_t * superblock);

void get_bgdt_from_group(
    e2_bgdt_t * bgdt, 
    uint32_t bg_block, 
    uint32_t group_no,
    uint32_t sectors_per_block
);




