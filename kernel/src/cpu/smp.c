#include "smp.h"
#include <stdint.h>
#include "../string.h"
#include "../kprintf.h"
#include "io.h" 
#include "../drivers/pit.h"
#include "../string.h"


uint8_t lapic_ids[256]={0}; // CPU core Local APIC IDs
uint8_t numcore=0;          // number of cores detected
uint64_t lapic_ptr=0;       // pointer to the Local APIC MMIO registers
uint64_t ioapic_ptr=0;      // pointer to the IO APIC MMIO registers

uint8_t bspid, bspdone = 0;      // BSP id and spinlock flag
volatile uint8_t aprunning = 0;  // count how many APs have started 
extern void ap_entry();

void ap_startup(int apicid){
    kprintf("Booted core #%d\n", apicid);
    for(;;);
}


void startup_aps(){

    // copy trampoline code to low address
    memcpy((void*) 0x8000, &ap_entry, 0x1000);

    __asm__ __volatile__ ("mov $1, %%eax; cpuid; shrl $24, %%ebx;": "=b"(bspid) : : );

    for(u8 i = 0; i < numcore; i++) {
        // do not start BSP, that's already running this code
        if(lapic_ids[i] == bspid) continue;
        // send INIT IPI
        *((volatile uint32_t*)(lapic_ptr + 0x280)) = 0;                                                                             // clear APIC errors
        *((volatile uint32_t*)(lapic_ptr + 0x310)) = 
            (*((volatile uint32_t*)(lapic_ptr + 0x310)) & 0x00ffffff) | (i << 24);                  // select AP
        *((volatile uint32_t*)(lapic_ptr + 0x300)) = 
            (*((volatile uint32_t*)(lapic_ptr + 0x300)) & 0xfff00000) | 0x00C500;          // trigger INIT IPI
        do { 
            __asm__ __volatile__ ("pause" : : : "memory"); }while(*((volatile uint32_t*)(lapic_ptr + 0x300)) & (1 << 12));         // wait for delivery

        *((volatile uint32_t*)(lapic_ptr + 0x310)) = 
            (*((volatile uint32_t*)(lapic_ptr + 0x310)) & 0x00ffffff) | (i << 24);         // select AP
	*((volatile uint32_t*)(lapic_ptr + 0x300)) = 
        (*((volatile uint32_t*)(lapic_ptr + 0x300)) & 0xfff00000) | 0x008500;          // deassert
        do { 
            __asm__ __volatile__ ("pause" : : : "memory"); 
        }while(*((volatile uint32_t*)(lapic_ptr + 0x300)) & (1 << 12));         // wait for delivery
	    //sleep(10);                                                                                                                 // wait 10 msec
        // send STARTUP IPI (twice)
	    for(u8 j = 0; j < 2; j++) {
	    	*((volatile uint32_t*)(lapic_ptr + 0x280)) = 0;                                                                     // clear APIC errors
	    	*((volatile uint32_t*)(lapic_ptr + 0x310)) = (*((volatile uint32_t*)(lapic_ptr + 0x310)) & 0x00ffffff) | (i << 24); // select AP
	    	*((volatile uint32_t*)(lapic_ptr + 0x300)) = (*((volatile uint32_t*)(lapic_ptr + 0x300)) & 0xfff0f800) | 0x000608;  // trigger STARTUP IPI for 0800:0000
	    	//udelay(200);                                                                                                        // wait 200 usec
            sleep(1);
	    	do { 
                __asm__ __volatile__ ("pause" : : : "memory"); 
            } while(*((volatile uint32_t*)(lapic_ptr + 0x300)) & (1 << 12)); // wait for delivery
	    }
    }

    bspdone = 1;

    kprintf("[SMP] %d Cores have been booted\n", aprunning);
}

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

    startup_aps();

}
