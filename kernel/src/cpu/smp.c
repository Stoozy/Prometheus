
#include <stdint.h>

#include "smp.h"
#include "io.h" 

#include "../string.h"
#include "../kprintf.h"
#include "../drivers/pit.h"
#include "../string.h"
#include "../config.h"

#include "../memory/pmm.h"


void ap_startup(u64 lapicid){

    __asm__ volatile ("cli");

#ifdef SMP_DEBUG
    kprintf("[SMP]  Booted core with lapic_id : #%llu\n", lapicid);
    kprintf("[SMP]  Halting...");
#endif

    for(;;);
}

void smp_init(struct stivale2_struct_tag_smp * smp_tag){

    u64 core_count = smp_tag->cpu_count;
    kprintf("[SMP]  Core Count: %u\n", core_count);

    for(u32 i=0; i < smp_tag->cpu_count; i++){
        struct stivale2_smp_info  current_cpu = smp_tag->smp_info[i];
        if(current_cpu.lapic_id != smp_tag->bsp_lapic_id){

#ifdef SMP_DEBUG
kprintf("[SMP]  Booting core with lapic_id : #%llu\n", current_cpu.lapic_id);
#endif
            current_cpu.target_stack = (u64)pmm_alloc_block();
            current_cpu.extra_argument = current_cpu.lapic_id;
            current_cpu.goto_address = (u64)&ap_startup;
        }
    }
}
