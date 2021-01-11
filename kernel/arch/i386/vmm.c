
#include <kernel/paging.h>
#include <kernel/vmm.h>
#include <kernel/pmm.h>

#include <string.h>

//! i86 architecture defines 1024 entries per table--do not change
#define PAGE_SIZE   4096
#define PAGES_PER_TABLE 1024
#define PAGES_PER_DIR	1024
#define PAGE_DIRECTORY_INDEX(x) (((x) >> 22) & 0x3ff)
#define PAGE_TABLE_INDEX(x) (((x) >> 12) & 0x3ff)
#define PAGE_GET_PHYSICAL_ADDRESS(x) (*x & ~0xfff)
#define PTABLE_ADDR_SPACE_SIZE 0x400000 //! page table represents 4mb address space
#define DTABLE_ADDR_SPACE_SIZE 0x100000000 //! directory table represents 4gb address space
#define PAGE_SIZE 4096 //! page sizes are 4k

enum PAGE_PTE_FLAGS {
 
	I86_PTE_PRESENT			=	1,		//0000000000000000000000000000001
	I86_PTE_WRITABLE		=	2,		//0000000000000000000000000000010
	I86_PTE_USER			=	4,		//0000000000000000000000000000100
	I86_PTE_WRITETHOUGH		=	8,		//0000000000000000000000000001000
	I86_PTE_NOT_CACHEABLE		=	0x10,		//0000000000000000000000000010000
	I86_PTE_ACCESSED		=	0x20,		//0000000000000000000000000100000
	I86_PTE_DIRTY			=	0x40,		//0000000000000000000000001000000
	I86_PTE_PAT			=	0x80,		//0000000000000000000000010000000
	I86_PTE_CPU_GLOBAL		=	0x100,		//0000000000000000000000100000000
	I86_PTE_LV4_GLOBAL		=	0x200,		//0000000000000000000001000000000
   	I86_PTE_FRAME			=	0x7FFFF000 	//1111111111111111111000000000000
};
 
//! page table
typedef struct page_table {
	page_t m_entries[PAGES_PER_TABLE];
} page_tab_t;
 
//! page directory
typedef struct page_dir {
	pde_t m_entries[PAGES_PER_DIR];
}page_dir_t;

page_dir_t * _cur_dir;


bool vmm_alloc_page(page_t * e){
	//! allocate a free physical frame
	void* p = pmm_alloc_block();
	if (!p) return false;
 
	//! map it to the page
	pte_set_frame (e, (physical_addr)p);
	pte_add_attrib (e, I86_PTE_PRESENT);
 
	return true;		
}

void vmm_free_page (page_t * e){
	void * p = (void *) pte_get_phys_addr(*e);
	if(p) pmm_free_block(p);
	pte_del_attrib(e, I86_PTE_PRESENT);
}

inline page_t* vmm_ptable_lookup_entry (page_table_t* p, virt_addr addr){
	if(p)	 return &p->m_entries[ PAGE_TABLE_INDEX (addr) ];
	return 0;
}

inline bool vmm_switch_pdirectory (pdirectory* dir) {
	if (!dir)
		return false;
 
	_cur_directory = dir;
	pmm_load_PDBR (_cur_pdbr);
	return true;
}

page_dir_t* vmm_get_directory(){
	return _cur_dir;
}

void vmm_flush_tlb_entry (virt_addr addr){
	asm("cli\n\t"
		"invlpg %%0\n\t"
		"sti": :"r" (addr) :);
}

void vmm_map_page (void* phys, void* virt){
   //! get page directory
   page_dir_t* pageDirectory = vmmngr_get_directory ();

   //! get page table
   pde_t* e = &pageDirectory->m_entries [PAGE_DIRECTORY_INDEX ((uint32_t) virt) ];

   //if ( (*e & I86_PTE_PRESENT) != I86_PTE_PRESENT) {
	if(e->present){
	  //! page table not present, allocate it
      page_tab_t* table = (page_tab_t*) pmmngr_alloc_block ();
      if (!table)
         return;

      //! clear page table
      memset (table, 0, sizeof(page_tab_t));

      //! create a new entry
      pde_t * entry =
         &pageDirectory->m_entries [PAGE_DIRECTORY_INDEX ( (uint32_t) virt) ];

      //! map in the table (Can also just do *entry |= 3) to enable these bits
      pde_add_attrib (entry, I86_PDE_PRESENT);
      pde_add_attrib (entry, I86_PDE_WRITABLE);
      pde_set_frame (entry, (phys_addr)table);
	}
}

void vmm_init(){

	//! allocate default page table
	page_tab_t* table = (page_tab_t*) pmm_alloc_block ();
	if (!table)
		return;
 
	//! allocates 3gb page table
	page_tab_t * table2 = (page_tab_t*) pmm_alloc_block ();
	if (!table2)
		return;

	//! clear page table
	vmmngr_ptable_clear(table);



	//! _st 4mb are idenitity mapped
	for (int i=0, frame=0x0, virt=0x00000000; i<1024; i++, frame+=4096, virt+=4096) {

 		//! create a new page

		page_t page=0;
		pte_add_attrib (&page, I86_PTE_PRESENT);
 		pte_set_frame (&page, frame);

		//! ...and add it to the page table
		table2->m_entries [PAGE_TABLE_INDEX (virt) ] = page;
	}
}
