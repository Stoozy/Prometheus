#include <fs/ext2.h>
#include <kernel/typedefs.h>
#include <stdio.h>
#include <drivers/ata.h>
#include <kernel/liballoc.h>

#include <string.h>


extern void kernel_panic(const char * reason);

static uint8_t bgdt_block = 1;

static e2_superblock_t superblock[4];

uint32_t get_block_size(e2_superblock_t * superblock){
    return 1024 << superblock->block_size;
} 

uint32_t get_inode_type(e2_inode_t * inode){
    return inode->mode & 0xF000;
}

uint32_t get_frag_size(e2_superblock_t * superblock){
    return 1024 << superblock->frag_size;
}

void get_bgdt_from_group(
        e2_bgdt_t * bgdt, 
        uint32_t bg_block,  
        uint32_t bg_no,
        uint32_t sectors_per_block
        )
{

    uint32_t bgdt_sector, bgdt_index;
    bgdt_sector = bg_block * sectors_per_block + bg_no / BGDT_PER_SECTOR ; 
    bgdt_index = bg_no % BGDT_PER_SECTOR;
    
    /*
     * No need to read the whole 
     * block (4096 bytes). Rather,
     * just read the correct sector.
     *
     */
    uint16_t * buffer = malloc(SECTOR_SIZE);

    read_sectors(buffer, 0xA0, bgdt_sector, 1);
    read_sectors(buffer, 0xA0, bgdt_sector, 1);
   
    memcpy(bgdt, ((uint8_t*)buffer)+(bgdt_index*BGDT_SIZE), BGDT_SIZE);

    free(buffer);

}


void dirents_dump(e2_inode_t * inode, uint32_t sectors_per_block){
    if(get_inode_type(inode) != EXT2_DIRENT){
        printf("Not a dir!");
        return;
    }
    
    printf("\n");

    uint8_t * buffer = malloc(SECTOR_SIZE);

    e2_dirent_t * dirent = malloc(sizeof(e2_dirent_t));
    uint8_t * dirent_addr = (uint8_t*)buffer;
    
    // dummy dirent
    dirent->total_size = 0;

    read_sectors((uint16_t*)buffer, 0xA0, inode->direct_block_ptrs[0] * sectors_per_block, 1);

    while(dirent->total_size < sizeof(e2_dirent_t)){
        memcpy(dirent, dirent_addr, sizeof(e2_dirent_t));

        printf("%s - %d bytes \n", dirent->name_chars, dirent->total_size);
        dirent_addr += dirent->total_size;
    }
    
    free(dirent);
    free(buffer);
}

void get_inode_from_bgdt(
        e2_inode_t * inode,
        e2_bgdt_t * bgdt, 
        uint32_t sectors_per_block,
        uint32_t index)
{
    // inodes start at 1
    --index;

    uint32_t inode_table = bgdt->inode_table_addr;

    uint32_t inode_sector = inode_table * sectors_per_block;
    inode_sector += (index * sizeof(e2_inode_t))/SECTOR_SIZE;

    uint16_t * buffer = malloc(SECTOR_SIZE);

    read_sectors(buffer, 0xA0, inode_sector, 1);
    read_sectors(buffer, 0xA0, inode_sector, 1);

    memcpy(inode, ((uint8_t*)buffer)+(index*sizeof(e2_inode_t)), sizeof(e2_inode_t));

    free(buffer);

}

static uint32_t number_of_block_groups(uint32_t total_blocks, uint32_t blocks_per_group){
    if(total_blocks % blocks_per_group != 0 )
        return (total_blocks/blocks_per_group)+1;
    return total_blocks/blocks_per_group;
}

void init_fs(uint32_t * sb_buf, uint8_t drive){
    uint32_t current_drive, sectors_per_block, inode_size = 128;

    // only set empty superblocks
    for(int i=0; i<4;++i)
        if(superblock[i].n_inodes == 0){
            memcpy((char*)&superblock[i], sb_buf, sizeof(superblock));
            current_drive = i;
            break;
        }

    sectors_per_block = get_block_size(&superblock[drive]) / SECTOR_SIZE;

    // verify ext2 fs
    if(superblock[current_drive].signature != 0xef53) {
        printf("Invalid fs (only supporting ext2 fs)\n");
        return;
    }

    if(superblock[current_drive].fs_state == FS_ERR_STATE){
        // handle error here
        // kernel_panic maybe ?

        printf("Filesystem is in an error state\n");
        //kernel_panic("Invalid Filesystem \n");
    }else{
        printf("Filesystem is clean (no errors)\n");
    }


    /* block group descriptor table is 
     * at block 1 by default.
     * It starts at block 2 only for 1024
     * bytes block size 
     */

    uint32_t bgdt_block = 1;

    if(get_block_size(&superblock[current_drive]) == 1024) bgdt_block = 2;


    if(superblock[current_drive].v_major >= 1) {
        // overwrite inode size
        // inode size defined in extended fields
        inode_size = superblock[current_drive].inode_size;
    }

    uint32_t n_block_groups = superblock[current_drive].n_blocks/superblock[current_drive].blocks_per_group;
    
    // get bgdt 
    
    uint32_t inode_block_group = (ROOT_INODE-1) / superblock[current_drive].inodes_per_group;

    e2_bgdt_t * bgdt = malloc(sizeof(e2_bgdt_t));
    get_bgdt_from_group(bgdt, bgdt_block, inode_block_group, sectors_per_block);

    printf("Block usage bitmap at block #%d\n", bgdt->block_usage_bitmap_addr);
    printf("Inode usage bitmap at block #%d\n", bgdt->inode_usage_bitmap_addr);

    printf("Number of dirs in current group: %d\n", bgdt->n_dirs);
    printf("Inode table at block #%d\n", bgdt->inode_table_addr);

    e2_inode_t * inode = malloc(sizeof(e2_inode_t));
    get_inode_from_bgdt(inode, bgdt, sectors_per_block, ROOT_INODE);

    printf("Mode 0x%x\n", inode->mode & 0xF000);
    printf("Size 0x%x\n", inode->size);

    printf("Root directory contents");
    dirents_dump(inode, sectors_per_block);
}


void fs_dump_info(uint8_t drive){


    printf("Signature: 0x%x\n", superblock[drive].signature);
    printf("Number of inodes : %d \n", superblock[drive].n_inodes);
    printf("Number of  blocks: %d \n", superblock[drive].n_blocks);

    printf("Number of block groups: %d\n", 
            superblock[drive].n_blocks/superblock[drive].blocks_per_group);

    printf("Blocks per group: %d\n", superblock[drive].blocks_per_group);
    printf("Inodes per group: %d\n", superblock[drive].inodes_per_group);
    printf("Frags per group: %d\n", superblock[drive].frags_per_group);

    printf("Block Size: %d bytes\n", get_block_size(&superblock[drive]) );
    printf("Fragment Size: %d bytes \n", get_frag_size(&superblock[drive]));

    printf("Unallocated blocks : %d\n", superblock[drive].n_unalloc_blocks);
    printf("Unallocated inodes : %d\n", superblock[drive].n_unalloc_inodes);

}





