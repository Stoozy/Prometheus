#include <kernel/pmm.h>
#include <kernel/vmm.h>
#include <kernel/util.h>

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

 
#define I86_PTE_PRESENT				1		    //0000000000000000000000000000001
#define I86_PTE_WRITABLE			2		    //0000000000000000000000000000010
#define I86_PTE_USER				4		    //0000000000000000000000000000100
#define I86_PTE_WRITETHOUGH			8		    //0000000000000000000000000001000
#define I86_PTE_NOT_CACHEABLE		0x10		//0000000000000000000000000010000
#define I86_PTE_ACCESSED			0x20		//0000000000000000000000000100000
#define I86_PTE_DIRTY				0x40		//0000000000000000000000001000000
#define I86_PTE_PAT			    	0x80		//0000000000000000000000010000000
#define I86_PTE_CPU_GLOBAL			0x100		//0000000000000000000000100000000
#define I86_PTE_LV4_GLOBAL			0x200		//0000000000000000000001000000000
#define I86_PTE_FRAME				0x7FFFF000 	//1111111111111111111000000000000
 
#define I86_PDE_PRESENT				1		    //0000000000000000000000000000001
#define I86_PDE_WRITABLE			2		    //0000000000000000000000000000010
#define I86_PDE_USER				4		    //0000000000000000000000000000100
#define I86_PDE_WRITETHOUGH			8		    //0000000000000000000000000001000
#define I86_PDE_NOT_CACHEABLE		0x10		//0000000000000000000000000010000
#define I86_PDE_ACCESSED			0x20		//0000000000000000000000000100000
#define I86_PDE_DIRTY				0x40		//0000000000000000000000001000000
#define I86_PDE_PAT			    	0x80		//0000000000000000000000010000000
#define I86_PDE_CPU_GLOBAL			0x100		//0000000000000000000000100000000
#define I86_PDE_LV4_GLOBAL			0x200		//0000000000000000000001000000000
#define I86_PDE_FRAME				0x7FFFF000 	//1111111111111111111000000000000

 
page_dir_t * _cur_dir;  // current page dir
phys_addr	 _cur_pdbr; // current page dir base reg


bool vmm_alloc_page(page_t * e){
    //! allocate a free physical frame
    void* p = pmm_alloc_block();
    if (!p) return false;
 
    //! map it to the page
    pte_set_frame (e, (phys_addr)p);
    pte_add_attrib (e, I86_PTE_PRESENT);
 
    return true;
}

void vmm_free_page (page_t * e){
    void * p = (void *) pte_get_phys_addr(e);
    if(p) pmm_free_block(p);
    pte_del_attrib(e, I86_PTE_PRESENT);
}

page_t* vmm_ptable_lookup_entry (page_table_t* p, virt_addr addr){
    if(p) return &p->m_entries[ PAGE_TABLE_INDEX (addr) ];
    return 0;
}

bool vmm_switch_pdirectory (page_dir_t* dir) {
    if (!dir)
        return false;

    _cur_dir = dir;
    pmm_load_PDBR (_cur_pdbr);
    return true;
}

page_dir_t* vmm_get_directory(){
    return _cur_dir;
}

void vmm_flush_tlb_entry (virt_addr addr){
    asm("cli\n\t"
        "invlpg (%0)\n\t"
        "sti\n\t": :"r" (addr) : "memory");
}

void vmm_map_page (void* phys, void* virt){
    //! get page directory
    page_dir_t* pageDirectory = vmm_get_directory ();

    //! get page table
    pde_t* e = &pageDirectory->m_entries [PAGE_DIRECTORY_INDEX ((uint32_t) virt) ];

    //if ( (*e & I86_PTE_PRESENT) != I86_PTE_PRESENT) {
    if(e->present){
      //! page table not present, allocate it
      page_table_t* table = (page_table_t*) pmm_alloc_block ();
      if (!table)
         return;

      //! clear page table
      memset (table, 0, sizeof(page_table_t));

      //! create a new entry
      pde_t * entry =
         &pageDirectory->m_entries [PAGE_DIRECTORY_INDEX ( (uint32_t) virt) ];

      //! map in the table (Can also just do *entry |= 3) to enable these bits
      pde_add_attrib (entry, I86_PDE_PRESENT);
      pde_add_attrib (entry, I86_PDE_WRITABLE);
      pde_set_frame (entry, (phys_addr)table);
    }
}

void vmm_ptable_clear(page_table_t * pt){
    memset(pt, 0, sizeof(page_table_t));
    return;
}

void vmm_init(){

    //! allocate default page table
    page_table_t* table = (page_table_t*) pmm_alloc_block ();
    if (!table)
        return;

    //! allocates 3gb page table
    page_table_t * table2 = (page_table_t*) pmm_alloc_block ();
    if (!table2)
        return;

    //! clear page table
    vmm_ptable_clear(table);


    //! _st 4mb are idenitity mapped
    for (int i=0, frame=0x0, virt=0x00000000; i<1024; i++, frame+=4096, virt+=4096) {

        //! create a new page

        page_t page={0};
        _set_bit(&page,0);
        _set_bit(&page,1);
        //pte_add_attrib (&page, I86_PTE_PRESENT);
        pte_set_frame (&page, frame);

        //! ...and add it to the page table
        table2->m_entries [PAGE_TABLE_INDEX (virt) ] = page;
    }

    //! create default directory table
    page_dir_t*	dir = (page_dir_t*) pmm_alloc_blocks (3);
    if (!dir)
        return;

    //! clear directory table and set it as current
    memset (dir, 0, sizeof (page_dir_t));

    pde_t* entry = &dir->m_entries [PAGE_DIRECTORY_INDEX (0xc0000000) ];
    pde_add_attrib (entry, I86_PDE_PRESENT);
    pde_add_attrib (entry, I86_PDE_WRITABLE);
    pde_set_frame (entry, (phys_addr)table);

    pde_t* entry2 = &dir->m_entries [PAGE_DIRECTORY_INDEX (0x00000000) ];
    pde_add_attrib (entry2, I86_PDE_PRESENT);
    pde_add_attrib (entry2, I86_PDE_WRITABLE);
    pde_set_frame (entry2, (phys_addr)table2);

    //! store current PDBR
    _cur_pdbr = (phys_addr) &dir->m_entries;

    //! switch to our page directory
    vmm_switch_pdirectory (&dir->m_entries);
    //! enable paging
    pmm_paging_enable (true);

}
