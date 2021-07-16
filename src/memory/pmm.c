#include <memory/pmm.h>
#include <util.h>

#include <string.h>
#include <stddef.h>
#include <kprintf.h>

#define BLOCK_SIZE          4096
#define BLOCKS_PER_BYTE     8
#define MAX_BITMAPS         131072

static u32     total_blocks;
static u32     total_bmaps;
static u32     free_blocks=0;
static u32     used_blocks=MAX_BITMAPS * BLOCK_SIZE;
static u32     last_checked_block = 1;
static u8      mmap[MAX_BITMAPS]; // max maps for a 4 GiB memory space



static void set_frame_used(u32 n_block){
    return set_bit(&mmap[n_block/8], n_block%8);
}

static void set_frame_free(u32 n_block){
    return clear_bit(&mmap[n_block/8], n_block%8);
}

static bool check_frame(u32 n_block){
    // 1 if used; 0 if free
    return check_bit(&mmap[n_block/8], n_block%8);
}


void pmm_init(){
    total_blocks = MAX_BITMAPS*BLOCKS_PER_BYTE;
    total_bmaps = total_blocks/BLOCKS_PER_BYTE;
    
    // every frame is used by default
    memset(&mmap[0], 0xff, total_bmaps);
}


void pmm_init_region(void * addr, size_t size){
    u32 start_frame = ((u32)addr)/BLOCK_SIZE;
    if(start_frame == 0){
        // start freeing from 4MiB
        start_frame = (1024*1024*4)/BLOCK_SIZE;
        kprintf("Starting at frame #%d instead of 0\n", start_frame);
    }

    u32 end_frame = start_frame + size/BLOCK_SIZE;
    
    for(u32 i=start_frame; i<end_frame; i++){
       set_frame_free(i);
       free_blocks++;
       used_blocks--;
    }

    return; 
}

static int pmm_get_first_free(){
    for(; last_checked_block<total_blocks; last_checked_block++){
        // check if fame index is free
        if(!(check_bit(mmap[last_checked_block/8],last_checked_block%8))){
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
static int pmm_get_first_free_chunk(u32 blocks){
    for(; last_checked_block<total_blocks; last_checked_block++){
        if(mmap[last_checked_block/8] != 0xff){

            for(u8 i=0; i<8; i++){

                u32 start_idx = last_checked_block*8+i;
                if(start_idx == total_blocks-1) return -1;
                // found free block
                if(!check_frame(start_idx)){

                    u32 free = 0; 
                    for(u32 j=start_idx; j<=(start_idx+blocks); j++){
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

void * pmm_alloc_blocks(u32 size){
    int sb = pmm_get_first_free_chunk(size); 

    if(sb == -1){
        /*
         * do another search beginning at frame 1 
         * to make sure no frames are left free
         */

        sb = pmm_get_first_free();
        if(sb == -1) return 0x0; // ran out of usable mem
    }
     
    u32 addr = sb * BLOCK_SIZE; 

    for(u32 b=sb; b<(sb+size); ++b){
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

    u32 addr = block * BLOCK_SIZE;

    // set block used
    set_frame_used(block);

    used_blocks++;
    free_blocks--;

    return  (void*) addr;
}


// TODO
void  pmm_free_blocks(u32 addr, u32 blocks);

void pmm_free_block(u32 addr){
    int block =  addr/BLOCK_SIZE;
    set_frame_free(block);

    used_blocks--;
    free_blocks++;
    return;
}


u32 pmm_get_free_block_count(){
    return free_blocks;
}

u32 pmm_get_block_count(){
    return total_blocks;
}

void pmm_dump(){
    for(u32 i=0; i<total_blocks; i++){
        if(check_frame(i))
            kprintf("Block #%d; Addr: 0x%x; Status: Used\n", i, i*BLOCK_SIZE );
        else 
            kprintf("Block #%d; Addr: 0x%x; Status: Free\n", i, i*BLOCK_SIZE );
    }

    for(u32 i=0; i<total_bmaps; i++){
        kprintf("Map #%d; Value: %u\n", i, mmap[i]);
    }
}

