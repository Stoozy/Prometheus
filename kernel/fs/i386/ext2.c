#include <fs/ext2.h>
#include <kernel/typedefs.h>
#include <stdio.h>
#include <drivers/ata.h>
#include <kernel/liballoc.h>

#include <string.h>

#define FS_ERR_STATE        1
#define FS_CLEAN_STATE      2


static uint8_t bgdt_block = 1;

static ext2fs_superblock_t superblock[4];

static uint32_t number_of_block_groups(uint32_t total_blocks, uint32_t blocks_per_group){
    if(total_blocks % blocks_per_group != 0 )
        return (total_blocks/blocks_per_group)+1;
    return total_blocks/blocks_per_group;
}

void init_fs(uint32_t * sb_buf){
    uint32_t current_drive = 0;

    // only set empty superblocks
    for(int i=0; i<4;++i)
        if(superblock[i].n_inodes == 0){
            memcpy((char*)&superblock[i], sb_buf, sizeof(superblock));
            current_drive = i;
            break;
        }


    // verify ext2 fs
    if(superblock[current_drive].signature != 0xef53) {
        printf("Invalid fs (only supporting ext2 fs)\n");
        return;
    }

    if(superblock[current_drive].fs_state == FS_ERR_STATE){
        // handle error here
        // kernel_panic maybe ?
    }else{
        printf("Filesystem is clean (no errors)\n");
    }

        //uint32_t block_groups = number_of_block_groups(blocks, blocks_per_group);
    //printf("%d block groups\n", block_groups);


    ///*
    // *  special case where the 
    // *  block descriptor table
    // *  starts at block 2
    // */
    //if(block_size == 1024){
    //    bgdt_block = 2;
    //}


    //uint16_t * read_buf = malloc(512*sizeof(char));

    //// dummy read
    //read_sectors(read_buf, 0xA0, bgdt_block * block_size/512, 1);
    //read_sectors(read_buf, 0xA0, bgdt_block * block_size/512, 1);

    //// each bgdt is offset by 32 bytes
    //
    //uint32_t * bgdt = (uint32_t *) read_buf;

    // 512 bytes per sector, 32 bytes per bgd
    // 512/32 = 16 bgds
    //for(int i=0; i<(512/32);++i){
    //    printf("Block Group #", i);
    //    uint32_t block_usage = bgdt[i*32+0];
    //    uint32_t inode_usage = bgdt[i*32+1];
    //    uint32_t inode_table_addr = bgdt[i*32+2];


    //    //printf("Block usage bitmap at block #%d\n", block_usage);
    //    //printf("Inode usage bitmap at block #%d\n", inode_usage);
    //    //printf("Inode Table address: 0x%x\n", inode_table_addr);

    //    printf("\n");
    //    Sleep(1000);
    //}

    // root dir inode is always 2
    // inode -1 / inodes_per_group 
    
    //uint32_t version = superblock[15] & 0x00ff  | superblock[19];

    //uint32_t inode_size = 128;
    //if(version >= 1 ){
    //    inode_size = superblock[22] & 0xff00;
    //}

    //printf("Inode size: %d\n", inode_size);

    //uint32_t root_inode_block_group = (1) / inodes_per_group;
    //uint32_t root_inode_index = (1) % inodes_per_group;

 
    //uint32_t root_inode_table_addr = bgdt[root_inode_block_group*5+2];
    //printf("Root inode table addr 0x%x\n", root_inode_table_addr);

    //uint32_t containing_block = (root_inode_index * inode_size)/ block_size;


    //printf("version %d\n", version);
    //read_sectors(read_buf, 0xA0, root_inode_table_addr * (block_size/512), 1);
    //read_sectors(read_buf, 0xA0, root_inode_table_addr * (block_size/512), 1);

    //uint16_t type = (read_buf[root_inode_index * (inode_size/1) + 0] & 0xF);
    //printf("Type is 0x%x\n", type);
    //
    //printf("Size of superblock: %d\n", sizeof(ext2fs_superblock_t));
    //printf("Size of bgd: %d\n", sizeof(ext2fs_block_group_desc_t));
    //printf("Size of inode: %d\n", sizeof(ext2fs_inode_t));

    //free(read_buf);
}


void fs_dump_info(uint8_t drive){


    printf("Signature: 0x%x\n", superblock[drive].signature);
    printf("Number of inodes : %d \n", superblock[drive].n_inodes);
    printf("Number of  blocks: %d \n", superblock[drive].n_blocks);

    printf("Block Size: %d bytes\n", 1024 << superblock[drive].block_size );
    printf("Fragment Size: %d bytes \n", 1024 << superblock[drive].frag_size);
    printf("Blocks per group: %d\n", superblock[drive].blocks_per_group);
    printf("Fragments per group : %d \n", superblock[drive].frags_per_group);
    printf("Inodes per group: %d \n\n", superblock[drive].inodes_per_group);

    printf("Unallocated blocks : %d\n", superblock[drive].n_unalloc_blocks);
    printf("Unallocated inodes : %d\n", superblock[drive].n_unalloc_inodes);

}





