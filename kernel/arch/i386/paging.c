#include <stdbool.h>
#include <kernel/paging.h>
#include <kernel/typedefs.h>

// bitset of frames (used or free)
uint32_t * frames;
uint32_t nframes;

// defined in kernel.c
extern uint32_t placement_address;

// Macros used in the bitset algorithms.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

static void set_frame(uint32_t frame_addr){
    uint32_t frame = frame_addr/4096;   // which frame? frames are each 4096 bytes
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off); // set bit 
}

static void clear_frame(uint32_t frame_addr){
    uint32_t frame = frame_addr/4096;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off); // clear bit
}

static uint32_t test_frame(uint32_t frame_addr){
    uint32_t frame = frame_addr/4096;
    uint32_t idx = INDEX_FROM_BIT(frame);
    uint32_t off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

// find the first frame
static uint32_t first_frame(){
    uint32_t i, j;
    for(i=0; i<INDEX_FROMT_BIT(nframes); i++){
        // at least one bit is free 
        for(j=0; j<32; j++){
            uint32_t to_test = 0x1 << j;
            if(!(frames[i]&to_test)) return i*32+j;
        }
    }
}


void alloc_frame(page_t *page, bool is_kernel, bool is_writable){
    if(page->frame !=0){
        return; // frame is already allocated
    }else{
        uint32_t idx = first_frame(); // get first frame
        if(idx == -1){
            while(1){
                printf("No free frames! ");
            }
        }
        set_frame(idx*4096);
        page->present = 1; // set in memory
        page->rw = is_writable;
        page->user = is_kernel;
        page->frame = idx;

    }
}

void free_frame(page_t *page){
    uint32_t frame;
    if(!(page->frame)){
        return; // the page frame is not actually allocated
    }else {
        clear_frame(frame);
        page->frame = 0x0;
    }
    
}



