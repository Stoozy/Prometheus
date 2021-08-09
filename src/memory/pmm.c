#include "pmm.h"
#include "../util.h"

#include "../string/string.h"
#include <stddef.h>
#include "../kprintf.h"
#include "../stivale.h"

#define BLOCK_SIZE          4096
#define BLOCKS_PER_BYTE     8
#define MAX_BITMAPS         262144 /* up to 64 GiB memory */ 

volatile u64     total_blocks = MAX_BITMAPS * BLOCKS_PER_BYTE; 
volatile u64     total_bmaps; 
volatile u64     free_blocks;
volatile u64     used_blocks; 
volatile u64     last_checked_block = 1; 
volatile u8      mmap[MAX_BITMAPS]; /* max maps for a 64 GiB memory space (takes 2 KiB) */

static void set_frame_used(u32 block){
    return set_bit((u8*)&mmap[block/8], block%8);
}

static void set_frame_free(u32 block){ return clear_bit((u8*)&mmap[block/8], block%8); }

static bool check_frame(u32 block){
    // 1 if used; 0 if free
    return check_bit((u8*)&mmap[block/8], block%8);
}


void pmm_init(struct stivale_struct * boot_info){

    total_bmaps = total_blocks/BLOCKS_PER_BYTE;
    used_blocks = total_blocks;
    free_blocks = 0;

    /* set every block to used status */
    memset((u8*)&mmap[0], 0xff, total_bmaps);


    /* Initializing different memory regions */
    kprintf("------------------Memory Information-------------------\n");

    struct stivale_mmap_entry * mmap_entries = 
        (struct stivale_mmap_entry * ) boot_info->memory_map_addr;

    for(int i=0; i<boot_info->memory_map_entries;++i){
        if(mmap_entries[i].type == STIVALE_MMAP_USABLE){
            pmm_init_region((void*) mmap_entries[i].base, mmap_entries[i].length);
            kprintf("[PMM]  Base 0x%x\n", mmap_entries[i].base);
            kprintf("[PMM]  Size %llu bytes\n", mmap_entries[i].length);
            kprintf("[PMM]  Type %d\n", mmap_entries[i].type);
        }
    }
}


void pmm_mark_region_used(void *  start_addr, void *  end_addr ){

    u64 start_block = (u64)start_addr / BLOCK_SIZE;
    u64 end_block = (u64)end_addr / BLOCK_SIZE + 1; 

    kprintf("Marking blocks from 0x%x to 0x%x as used\n", 
            start_block * BLOCK_SIZE, end_block * BLOCK_SIZE);

    u64 block = start_block;
    for(; block < end_block; ++block){
        set_frame_used(block);
    }

    /* update globals */
    --free_blocks;
    ++used_blocks;

    return;
}

void pmm_init_region(void * addr, u64 size){

    kprintf("[PMM]  address: 0x%x with size: %llu bytes.\n", addr, size);

    u64 start_frame = ((u64)addr)/BLOCK_SIZE;
    u64 end_frame = start_frame + (size/BLOCK_SIZE);
    
    for(u64 i=start_frame; i<end_frame; i++){
       set_frame_free(i);
       ++free_blocks;
       --used_blocks;
    }

    kprintf("[PMM]  Free Blocks: %lu\n", free_blocks);
    kprintf("[PMM]  Blocks: %lu\n\n", used_blocks);

    return; 
}

static int pmm_get_first_free(){
    for(; last_checked_block<total_blocks; last_checked_block++){
        // check if fame index is free
        if(!(check_bit((u8*)&mmap[last_checked_block/8],last_checked_block%8))){
            last_checked_block++;
            return last_checked_block-1;
        }
    }
    last_checked_block = 1;
    return -1;
}

/*
 * Get contiguous blocks
 *
 */
static int pmm_get_first_free_chunk(u64 blocks){
    for(; last_checked_block<total_blocks; last_checked_block++){
        if(mmap[last_checked_block/8] != 0xff){
            for(u8 i=0; i<8; i++){
                u64 start_idx = last_checked_block * 8 + i;
                if(start_idx == total_blocks-1) return -1;
                // found free block
                if(!check_frame(start_idx)){
                    u64 free = 0; 
                    for(u64 j=start_idx; j<=(start_idx+blocks); j++){
                        // used
                        if(check_frame(j)){
                            start_idx++;
                            break;
                        }
                        else free++;
                        if(free == blocks) return start_idx;
                    }
                } else start_idx++;
            }
        }
    }

    last_checked_block = 1;
    return -1;
}

void * pmm_alloc_blocks(u64 size){
    int sb = pmm_get_first_free_chunk(size); 

    if(sb == -1){
        /*
         * do another search beginning at frame 1 
         * to make sure no frames are left free
         */

        sb = pmm_get_first_free();
        if(sb == -1) return 0x0; // ran out of usable mem
    }
     
    u64 addr = sb * BLOCK_SIZE; 

    for(u64 b=sb; b<(sb+size); ++b){
        set_frame_used(b);
    }

    used_blocks += size;
    free_blocks -= size;

    return (void*) addr;
}

void * pmm_alloc_block(){
    int block = pmm_get_first_free(); 

    if(block == -1){
        /*
         * do another search beginning at frame 1 
         * to make sure no frames are left free
         */
        block = pmm_get_first_free();
        if(block == -1) return 0x0; // ran out of usable mem
    } 

    u64 addr = block * BLOCK_SIZE;

    // set block used
    set_frame_used(block);

    used_blocks++;
    free_blocks--;

    return  (void*) addr;
}


// TODO
void  pmm_free_blocks(u64 addr, u64 blocks);

void pmm_free_block(u64 addr){
    int block = addr/BLOCK_SIZE;

    set_frame_free(block);

    used_blocks--;
    free_blocks++;
    return;
}


u64 pmm_get_free_block_count(){
    return free_blocks;
}

u64 pmm_get_block_count(){
    return total_blocks;
}



void pmm_dump(){

    kprintf("---------------------PMM Information-------------------\n");
    kprintf("[PMM]  %lu total blocks\n", total_blocks);
    kprintf("[PMM]  %lu free blocks\n", free_blocks);
    kprintf("[PMM]  %lu used blocks\n", used_blocks);

    //for(u64 i=0; i<total_blocks; i++){
    //    if(check_frame(i))
    //        kprintf("Block #%d; Addr: 0x%x; Status: Used\n", i, i*BLOCK_SIZE );
    //    else 
    //        kprintf("Block #%d; Addr: 0x%x; Status: Free\n", i, i*BLOCK_SIZE );
    //}

    /*for(u64 i=0; i<total_bmaps; i++){
    }
    */

}

