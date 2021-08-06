#ifndef _VMM_H
#define _VMM_H 1

#include "../typedefs.h"
#include <stdbool.h>

typedef struct {
    u64 pml4i;
    u64 pml3i;
    u64 pml2i;
    u64 pml1i;
} __attribute__((packed)) PageIndex;

typedef struct {
    bool present : 1;
    bool writable : 1;
    bool user : 1;
    bool write_through : 1;
    bool cache : 1;
    bool accessed : 1;
    int zero0: 6;
    u64 physical_address : 36;
    int zero1: 15;
    bool execute_disabled : 1;
} __attribute__((packed)) Pml4e;

typedef struct {
    bool present : 1;
    bool writable : 1;
    bool user : 1;
    bool write_through : 1;
    bool cache : 1;
    bool accessed : 1;
    int zero0: 1;
    int size : 1;       /* must be 0 otherwise, this entry maps a 1GB page. */
    int zero1: 4;           
    u64 physical_address : 36;
    int zero2: 15;
    bool execute_disabled : 1;
} __attribute__((packed)) Pml3e;

typedef struct {
    bool present : 1;               // Must be 1 to reference a PML-1
    bool writable : 1;              // If 0, writes may not be allowed.
    bool user : 1;                  // If 0, user-mode accesses are not allowed
    bool write_thought : 1;         // Page-level write-through
    bool cache : 1;                 // Page-level cache disable
    bool accessed : 1;              // Indicates whether this entry has been used
    int zero0 : 1;                  // Ignored
    int size : 1;                   // Must be 0 otherwise, this entry maps a 2-MByte page.
    int zero1 : 4;                  // Ignored
    u64 physical_address : 36; // Physical address of a 4-KByte aligned PLM-1
    int zero2 : 15;                 // Ignored
    bool execute_disabled : 1;      // If IA32_EFER.NXE = 1, Execute-disable

}__attribute__((packed)) Pml2e;

typedef struct  {
    bool present : 1;             
    bool writable : 1;            
    bool user : 1;                
    bool write_thought : 1;       
    bool cache : 1;               
    bool accessed : 1;            
    int dirty : 1;                
    int memory_type : 1;          
    int global : 1;               
    int zero0 : 3;                
    u64 physical_address : 36;
    int zero1 : 10;               
    bool execute_disabled : 1;    
} __attribute__((packed)) Pml1e;

typedef struct {
    Pml4e entries[512];
} __attribute__((packed)) PML4;


typedef struct {
    Pml3e entries[512];
} __attribute__((packed)) PML3;


typedef struct {
    Pml2e entries[512];
} __attribute__((packed)) PML2;


typedef struct  {
    Pml1e entries[512];
} __attribute__((packed)) PML1;



i32 vmm_init();
i32 vmm_map(void * p_virtual, void * p_physical);
PageIndex vmm_get_index(u64 vaddr);

#endif 
