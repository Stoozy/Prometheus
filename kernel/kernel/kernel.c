#include <stdbool.h>
#include <stdio.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

void Sleep(uint32_t ms);


static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}

static inline void io_wait(void)
{
    /* TODO: This is probably fragile. */
    asm volatile ( "jmp 1f\n\t"
                   "1:jmp 2f\n\t"
                   "2:" );
}
static inline bool are_ints_enabled(){
    uint64_t flags;
    asm volatile("pushf\n\t" "pop %0" : "=g"(flags));
    return flags & (1 << 9);
}

void read_CMOS(uint8_t *arr){
  unsigned char tvalue, index;
 
    for(index = 0; index < 128; index++)
    {
        asm("cli");
        outb(0x70, index);
        uint8_t in = inb(0x71);
        asm("sti");
         
        arr[index] = tvalue;
    }
}

void kernel_main(void) {
    uint8_t time[128];
	terminal_initialize();

    init_gdt();
    init_idt();


    bool interrupt_status  = are_ints_enabled();
   
    if(interrupt_status) printf("Interrupt requests are currently enabled\n");
    else printf("Interrupt requests are currently disabled\n");

    terminal_setcolor(0xE); // yellow
    printf("%s"," _ _ _     _                      _              _____ _____ \n");
    printf("%s","| | | |___| |___ ___ _____ ___   | |_ ___    ___|     |   __|\n");
    printf("%s","| | | | -_| |  _| . |     | -_|  |  _| . |  |- _|  |  |__   |\n");
    printf("%s","|_____|___|_|___|___|_|_|_|___|  |_| |___|  |___|_____|_____|\n");
    terminal_setcolor(0xF); // white

    printf("\n");
    //read_CMOS(time);
    
    outb(0x70, (0<<7) | (0x02));
    uint8_t min = inb(0x71);

    outb(0x70, (0<<7) | (0x04));
    uint8_t hour = inb(0x71);

    printf("Time: %d:%d", hour, min);
    //for(int i=0; i<128; i++){
    //    printf("%d ", time[i]);
    //}

    for(;;){
        asm ("hlt");
    }
}
