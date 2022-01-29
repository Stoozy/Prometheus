#include <stdint.h>

#include "smp.h"
#include "io.h" 

#include "../kprintf.h"
#include "../drivers/pit.h"
#include "../string/string.h"
#include "../config.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"

#include "../memory/pmm.h"

#define LOCKED_READ(VAR) ({    \
    typeof(VAR) ret = 0;       \
    asm volatile (             \
        "lock xadd %0, %1"     \
        : "+r"(ret), "+m"(VAR) \
        :                      \
        : "memory"             \
    );                         \
    ret;                       \
})

#define LOCKED_INC(VAR) ({ \
    bool ret;              \
    asm volatile (         \
        "lock inc %1"      \
        : "=@ccnz" (ret)   \
        : "m" (VAR)        \
        : "memory"         \
    );                     \
    ret;                   \
})

#define LOCKED_WRITE(VAR, VAL) ({ \
    typeof(VAR) ret = VAL;        \
    asm volatile (                \
        "lock xchg %1, %0"        \
        : "+r"(ret), "+m"(VAR)    \
        :                         \
        : "memory"                \
    );                            \
    ret;                          \
})


static size_t alive_cpus = 1; // BSP already running

void ap_startup(){
    __asm__ volatile ("cli");
    // increment alive cpus
    //LOCKED_INC(alive_cpus);
    

    for(;;);
}

void smp_init(struct stivale2_struct_tag_smp * smp_tag){

    u64 core_count = smp_tag->cpu_count;
    kprintf("[SMP]  Core Count: %u\n", core_count);

    for(u32 i=0; i < smp_tag->cpu_count; i++){

        if(smp_tag->smp_info[i].lapic_id != smp_tag->bsp_lapic_id){
#ifdef SMP_DEBUG
kprintf("[SMP]  Booting core with LAPIC ID : #%llu\n", smp_tag->smp_info[i].lapic_id);
#endif
            LOCKED_WRITE(smp_tag->smp_info[i].target_stack, (u64)pmm_alloc_block()+0x1000);

            LOCKED_WRITE(smp_tag->smp_info[i].goto_address, (u64)&ap_startup);
        }
    }

    while(LOCKED_READ(alive_cpus) != smp_tag->cpu_count);
    // success!
    kprintf("[SMP]  All cpus are alive and running!\n");

    for(;;);
}
