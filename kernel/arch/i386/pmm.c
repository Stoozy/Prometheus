#include <kernel/pmm.h>
#include <kernel/util.h>

#include <string.h>
#include <stddef.h>

#define BLOCK_SIZE          4096
#define BLOCKS_PER_BYTE     8
#define MAX_BITMAPS         131072

static uint32_t     total_blocks;
static uint32_t     total_bmaps;
static uint32_t     free_blocks=0;
static uint32_t     used_blocks=MAX_BITMAPS * BLOCK_SIZE;
static uint32_t     last_checked_block = 1;
static uint8_t      mmap[MAX_BITMAPS]; // max maps for a 4 GiB memory space



static void set_frame_used(uint32_t n_block){
    return _set_bit(&mmap[n_block/8], n_block%8);
}

static void set_frame_free(uint32_t n_block){
    return _clear_bit(&mmap[n_block/8], n_block%8);
}

static bool check_frame(uint32_t n_block){
    // 1 if used; 0 if free
    return _check_bit(&mmap[n_block/8], n_block%8);
}

void Sleep(uint32_t ms);

void pmm_init(){
    total_blocks = MAX_BITMAPS*BLOCKS_PER_BYTE;
    total_bmaps = total_blocks/BLOCKS_PER_BYTE;
    
    // every frame is used by default
    memset(&mmap[0], 0xff, total_bmaps);
}


void pmm_init_region(void * addr, size_t size){
    uint32_t start_frame = ((uint32_t)addr)/BLOCK_SIZE;
    if(start_frame == 0){
        // start freeing from 4MiB
        start_frame = (1024*1024*4)/BLOCK_SIZE;
        printf("Starting at frame #%d instead of 0\n", start_frame);
    }

    uint32_t end_frame = start_frame + size/BLOCK_SIZE;
    
    for(uint32_t i=start_frame; i<end_frame; i++){
       set_frame_free(i);
       free_blocks++;
       used_blocks--;
    }

    return; 
}

static int pmm_get_first_free(){
    for(; last_checked_block<total_blocks; last_checked_block++){
        // check if fame index is free
        if(!(_check_bit(mmap[last_checked_block/8],last_checked_block%8))){
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
static int pmm_get_first_free_chunk(uint32_t blocks){
    for(; last_checked_block<total_blocks; last_checked_block++){
        if(mmap[last_checked_block/8] != 0xff){

            for(uint8_t i=0; i<8; i++){

                uint32_t start_idx = last_checked_block*8+i;
                if(start_idx == total_blocks-1) return -1;
                // found free block
                if(!check_frame(start_idx)){

                    uint32_t free = 0; 
                    for(uint32_t j=start_idx; j<=(start_idx+blocks); j++){
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

void * pmm_alloc_blocks(uint32_t size){
    int sb = pmm_get_first_free_chunk(size); 

    if(sb == -1){
        /*
         * do another search beginning at frame 1 
         * to make sure no frames are left free
         */

        sb = pmm_get_first_free();
        if(sb == -1) return 0x0; // ran out of usable mem
    }
     
    uint32_t addr = sb * BLOCK_SIZE; 

    for(uint32_t b=sb; b<(sb+size); ++b){
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

    uint32_t addr = block * BLOCK_SIZE;

    // set block used
    set_frame_used(block);

    used_blocks++;
    free_blocks--;

    return  (void*) addr;
}


// TODO
void  pmm_free_blocks(uint32_t addr, uint32_t blocks);

void pmm_free_block(uint32_t addr){
    int block =  addr/BLOCK_SIZE;
    set_frame_free(block);

    used_blocks--;
    free_blocks++;
    return;
}


uint32_t pmm_get_free_block_count(){
    return free_blocks;
}

uint32_t pmm_get_block_count(){
    return total_blocks;
}

void pmm_dump(){
    for(uint32_t i=0; i<total_blocks; i++){
        if(check_frame(i))
            printf("Block #%d; Addr: 0x%x; Status: Used\n", i, i*BLOCK_SIZE );
        else 
            printf("Block #%d; Addr: 0x%x; Status: Free\n", i, i*BLOCK_SIZE );
        Sleep(300);
    }

    for(uint32_t i=0; i<total_bmaps; i++){
        printf("Map #%d; Value: %u\n", i, mmap[i]);
    }
}


