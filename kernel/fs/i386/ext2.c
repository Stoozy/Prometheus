#include <fs/ext2.h>
#include <kernel/typedefs.h>
#include <stdio.h>
#include <drivers/ata.h>
#include <kernel/liballoc.h>

#define FS_ERR_STATE        1
#define FS_CLEAN_STATE      2

static uint8_t bgdt_block = 1;

void init_fs(uint32_t * superblock){
    uint32_t inodes = superblock[0];
    uint32_t blocks = superblock[1];

    uint32_t signature = superblock[14] & 0xffff;
    uint32_t fs_state = superblock[14] & 0xffff;

    uint32_t block_size = 1024 << superblock[6];
    uint32_t fragment_size = 1024 << superblock[7];

    uint32_t blocks_per_group = superblock[8];
    uint32_t frags_per_group = superblock[9];
    uint32_t inodes_per_group = superblock[10];

    uint32_t unallocated_blocks = superblock[3];
    uint32_t unallocated_inodes = superblock[4];

    // verify ext2 fs
    if(signature != 0xef53) {
        printf("Invalid fs (only supporting ext2 fs)\n");
        return;
    }

    printf("Valid ext2 fs!\n");
    printf("Signature: 0x%x\n", signature);
    printf("Number of inodes : %d \n", inodes);
    printf("Number of  blocks: %d \n", blocks);

    if(fs_state == FS_ERR_STATE){
        // handle error here
        // kernel_panic maybe ?
    }else{
        printf("Filesystem is clean (no errors)\n");
    }


    printf("Block Size: %d bytes\n", block_size);
    printf("Fragment Size: %d bytes \n", fragment_size );
    printf("Blocks per group: %d\n", blocks_per_group);
    printf("Fragments per group : %d \n", frags_per_group);
    printf("Inodes per group: %d \n\n", inodes_per_group);

    printf("Unallocated blocks : %d\n", unallocated_blocks);
    printf("Unallocated inodes : %d\n", unallocated_inodes);

    // spcial case where the 
    // block descriptor table
    // starts at block 2
    if(block_size == 1024){
        bgdt_block = 2;
    }

    uint16_t * buf = malloc(512*sizeof(char));

    read_sectors(buf, 0xA0, bgdt_block, 1);

    uint32_t * bgdt = (uint32_t *) buf;

    uint32_t block_usage = bgdt[0];
    uint32_t inode_usage = bgdt[1];
    uint32_t inode_table_addr = bgdt[2];

    printf("Block usage bitmap: %d\n", block_usage);
    printf("Inode usage bitmap: %d\n", inode_usage);
    printf("Inode Table address: 0x%x\n", inode_table_addr);

}




