#define BLOCK_SIZE          4096   
#define BLOCKS_PER_BYTE     8 

#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#include <kernel/pmm.h>
#include <kernel/typedefs.h>


static	uint32_t	_mmngr_mem_size=0; //! size of physical memory
static	uint32_t	_mmngr_used_blocks=0; //! number of blocks currently in use
static	uint32_t	_mmngr_max_blocks=0; //! maximum number of available memory blocks
static	uint32_t*	_mmngr_mmap= 0; //! memory map bit array. Each bit represents a memory block

inline void mmap_unset (int bit) { 
  _mmngr_mmap[bit / 32] &= ~ (1 << (bit % 32));
}// unset block by unsetting corresponding bit

inline void mmap_set (int bit) { 
  _mmngr_mmap[bit / 32] |= (1 << (bit % 32));
}// set block by setting corresponding bit


inline bool mmap_is_set (int bit) { 
 return _mmngr_mmap[bit / 32] &  (1 << (bit % 32));
} // checks if bit in bitmap is set


static inline uint32_t _pmm_get_block_count(){
	return _mmngr_max_blocks;
}

uint32_t pmm_get_block_count(){
	return _pmm_get_block_count();
}


static inline uint32_t _pmm_get_free_block_count(){
	return _mmngr_max_blocks - _mmngr_used_blocks;
}
/*
 *  bitmap is the phys_addr where pmm keeps it's bitmap struct
 *  mem_size is in KiB
 */
void pmm_init(size_t mem_size, phys_addr bitmap){
	_mmngr_mem_size = mem_size;
	_mmngr_mmap	=	(uint32_t*) bitmap;
	_mmngr_used_blocks = _pmm_get_block_count(); 
	_mmngr_max_blocks =  _pmm_get_block_count(); 

	//! By default, all of memory is in use
	memset (_mmngr_mmap, 0xf, _pmm_get_block_count() / BLOCKS_PER_BYTE);
}


/*
 *  initalize physical memory region
 */ 
void pmm_init_region(phys_addr base, size_t size){
	uint8_t bit = base/BLOCK_SIZE;
	uint8_t blocks = size/BLOCK_SIZE;

	for (; blocks>0; blocks--) {
		// free the memory
		mmap_unset (bit++); 
		_mmngr_used_blocks--;
	}
 
	mmap_set (0);	//first block is always set. This insures allocs cant be 0
}


void pmm_destroy_region(phys_addr base, size_t size){
	uint8_t bit = base/BLOCK_SIZE;
	uint8_t blocks = size/BLOCK_SIZE;

	for (; blocks>0; blocks--) {
		//mark block used
		mmap_set(bit++); 
		_mmngr_used_blocks++;
	}
}

int pmm_get_first_free(){
	//! find the first free bit
	for (uint32_t i=0; i< _pmm_get_block_count() / 32; i++)
		if (_mmngr_mmap[i] != 0xffffffff)
			for (int j=0; j<32; j++) {		//! test each bit in the dword
 
				if(!(mmap_is_set(j)))
					return i*32+j;
				//int bit = 1 << j;
				//if (! (_mmngr_memory_map[i] & bit) )
					//return i*32+j;
			}
	// no free blocks available 
	return -1;
}


void * pmm_alloc_block(){
	if (_pmm_get_free_block_count() <= 0)
		return 0;	//out of memory
 
	int frame = pmm_get_first_free();
 
	if (frame == -1)
		return 0;	//out of memory
 
	mmap_set(frame);
 
	phys_addr addr = frame * BLOCK_SIZE;
	_mmngr_used_blocks++; 

	return (void*)addr;
}

void pmm_free_block(void * ptr){
	phys_addr addr = (phys_addr) ptr;
	int frame = addr / BLOCK_SIZE;
 
	mmap_unset(frame);
	_mmngr_used_blocks--;
}



