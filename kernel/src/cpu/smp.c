#include "smp.h"
#include <stdint.h>
#include "../string.h"
#include "../kprintf.h"

uint8_t lapic_ids[256]={0}; // CPU core Local APIC IDs
uint8_t numcore=0;          // number of cores detected
uint64_t lapic_ptr=0;       // pointer to the Local APIC MMIO registers
uint64_t ioapic_ptr=0;      // pointer to the IO APIC MMIO registers
 

void detect_cores(u8 * rsdt){
  uint8_t *ptr, *ptr2;
  uint32_t len;
 
  // iterate on ACPI table pointers
  for(len = *((uint32_t*)(rsdt + 4)), ptr2 = rsdt + 36; ptr2 < rsdt + len; ptr2 += rsdt[0]=='X' ? 8 : 4) {
    ptr = (uint8_t*)(uintptr_t)(rsdt[0]=='X' ? *((uint64_t*)ptr2) : *((uint32_t*)ptr2));
    if(!memcmp(ptr, "APIC", 4)) {
      // found MADT
      lapic_ptr = (uint64_t)(*((uint32_t*)(ptr+0x24)));
      ptr2 = ptr + *((uint32_t*)(ptr + 4));
      // iterate on variable length records
      for(ptr += 44; ptr < ptr2; ptr += ptr[1]) {
        switch(ptr[0]) {
          case 0: if(ptr[4] & 1) lapic_ids[numcore++] = ptr[3]; break; // found Processor Local APIC
          case 1: ioapic_ptr = (uint64_t)*((uint32_t*)(ptr+4)); break;  // found IOAPIC
          case 5: lapic_ptr = *((uint64_t*)(ptr+4)); break;             // found 64 bit LAPIC
        }
      }
      break;
    }
  }
}

void smp_init(RSDPDescriptor * rsdp){

    kprintf("[ACPI] RSDP version : %u\n", rsdp->Revision);
    kprintf("[ACPI] RSDP signature : %s\n", rsdp->Signature);
    kprintf("[ACPI] RSDT Address : %x\n", rsdp->RsdtAddress);


    detect_cores((u8*)rsdp->RsdtAddress);

    kprintf("Found %d cores, IOAPIC %lx, LAPIC %lx, Processor IDs:", numcore, ioapic_ptr, lapic_ptr);
    for(u8 i = 0; i < numcore; i++)
      kprintf(" %d", lapic_ids[i]);
    kprintf("\n");
}
