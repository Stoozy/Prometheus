#include <fs/ext2.h>
#include <kernel/typedefs.h>
#include <stdio.h>
#include <drivers/ata.h>
#include <kernel/liballoc.h>

#include <string.h>

#include <fs/fs.h>


extern void kernel_panic(const char * reason);

/* block group descriptor table is 
 * at block 1 by default.
 * It starts at block 2 only for 1024
 * bytes block size 
 */

static uint8_t bgdt_block = 1;

static e2_superblock_t superblock[4];
static uint32_t inodes_per_group = 0; 
static uint32_t sectors_per_block = 0;
static uint32_t block_size = 4096;

uint32_t get_block_size(e2_superblock_t * superblock){
    return 1024 << superblock->block_size;
} 

dirent_t * ext2_mount(fs_type_t * type){
    // root node is always 2
    dirent_t * root_dir = malloc(sizeof(dirent_t));
    memcpy(root_dir->name, "/", sizeof("/"));
    //root_dir->name = "/";
    root_dir->inode = ROOT_INODE;

    return root_dir;
}

void ext2_umount(){
    printf("Unmounted");
    return;
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


void dump_dirent(e2_inode_t * inode, uint32_t sectors_per_block){

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

        // set end of name
        dirent->name_chars[dirent->name_len] = '\0';

        printf("Inode #%d: %s\n", dirent->inode,  dirent->name_chars);

        if(dirent->type == EXT2_DIRENT_FILE) dump_file(dirent, sectors_per_block);
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

    memcpy(inode, ((uint8_t*)buffer)+(index*sizeof(e2_inode_t)%SECTOR_SIZE), sizeof(e2_inode_t));

    free(buffer);
}


void get_inode_from_index(e2_inode_t * inode_ptr, uint32_t inode){

    /* get inode group */
    uint32_t inode_block_group = (inode-1) / inodes_per_group;
    uint32_t index = (inode-1) % inodes_per_group;

    /* increase sectors to read based on overflowing data */
    e2_bgdt_t * bgdt = malloc(sizeof(e2_bgdt_t));
    get_bgdt_from_group(bgdt, bgdt_block, inode_block_group, sectors_per_block);

    get_inode_from_bgdt(inode_ptr, bgdt, sectors_per_block, inode);

    free(bgdt);
}

static uint32_t number_of_block_groups(uint32_t total_blocks, uint32_t blocks_per_group){
    if(total_blocks % blocks_per_group != 0 )
        return (total_blocks/blocks_per_group)+1;
    return total_blocks/blocks_per_group;
}


void dump_file(e2_dirent_t * dirent, uint32_t sectors_per_block){
    if(dirent->type != EXT2_DIRENT_FILE){
        printf("Not a valid file");
        return;
    }
    dirent->name_chars[dirent->name_len] = '\0';

    printf("Contents of %s : \n", dirent->name_chars);

    e2_inode_t * inode = malloc(sizeof(e2_inode_t));
    get_inode_from_index(inode, dirent->inode);

    uint8_t * bytes = malloc(inode->size);


    uint32_t sectors_to_read = inode->size/SECTOR_SIZE;
    if(inode->size % SECTOR_SIZE != 0) sectors_to_read++;

    uint32_t blocks_to_read = inode->size/block_size;
    if(dirent->total_size % block_size != 0) blocks_to_read++;
    
    uint16_t * buffer = malloc(blocks_to_read * block_size);

    printf("Reading %d block(s)  \n", blocks_to_read);
    for(int i=0; i<blocks_to_read; ++i){
        if(inode->direct_block_ptrs[i] != 0 ){
            printf("Reading %d sectors  \n", sectors_to_read);

            read_sectors(buffer, 0xA0, inode->direct_block_ptrs[i] * sectors_per_block, sectors_per_block);
            read_sectors(buffer, 0xA0, inode->direct_block_ptrs[i] * sectors_per_block, sectors_per_block);

            memcpy(bytes, buffer, inode->size);

            printf("%s", bytes);

            Sleep(2000);
        }
    }

    free(inode);
    free(bytes);
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
    inodes_per_group = superblock[drive].inodes_per_group;
    block_size = get_block_size(&superblock[drive]);

    // verify ext2 fs
    if(superblock[current_drive].signature != 0xef53) {
        printf("Invalid fs (only supporting ext2 fs)\n");
        return;
    }

    if(superblock[current_drive].fs_state == FS_ERR_STATE){
        // TODO: handle error here

        printf("Filesystem is in an error state\n");
        
        switch(superblock[current_drive].err_action){
            case 0: 
                printf("Continuing ...");
                break;
            case 1:
                printf("Mount as RO");
                break;
            case 2:
                kernel_panic("Invalid Filesystem");
                break;
            default:
                break;
        }

    }else{
        printf("Filesystem is clean (no errors)\n");
    }

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

    fs_type_t * ext2_fs = malloc(sizeof(fs_type_t));

    memcpy(ext2_fs->name, "ext2\0", sizeof("ext2\0"));
    ext2_fs->mount = &ext2_mount;
    ext2_fs->umount = &ext2_umount;

    register_fs(ext2_fs);

    //printf("Root directory contents");
    dump_dirent(inode, sectors_per_block);
}


void fs_dump_info(uint8_t drive){

    printf("\nFilesystem information: \n");
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





