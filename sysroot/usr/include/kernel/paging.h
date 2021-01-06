#include <kernel/typedefs.h>

typedef struct page{
    uint32_t present    :1; // page present in memory  
    uint32_t rw         :1; // r if clear, rw if set
    uint32_t user       :1; // supervisor level only if clear
    uint32_t accessed   :1; // has the page been accessed since last refresh?
    uint32_t dirty      :1; // written to since last refresh?
    uint32_t unused     :7;
    uint32_t frame      :20;
}   page_t;


struct page_table{
   page_t pages[1024];
} page_table_t;

typedef struct page_directory{
	// pointers to tables
	page_table_t *tables[1024];
	/**
	  Array of pointers to the pagetables above, but gives their *physical*
	  location, for loading into the CR3 register.
	**/
	u32int tablesPhysical[1024];
	/**
	  The physical address of tablesPhysical. This comes into play
	  when we get our kernel heap allocated and the directory
	  may be in a different location in virtual memory.
	**/
	u32int physicalAddr;
} page_directory_t;

// initialize paging
void init_paging();

/*
*	causes page dir to be loaded to the cr3 register 
*/
void switch_page_dir(page_directory_t *new);

// get page pointer to the page required
page_t *get_page(uint32_t addr, int make, page_directory_t *dir);

// handle page faults
void page_fault(registers_t regs);

