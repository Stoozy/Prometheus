#include <kernel/typedefs.h>
#include <kernel/pci.h>
#include <stdio.h>
#include <kernel/io.h>
#include <stdlib.h>
#include <kernel/idt.h>

static device_t storage_controller;
static device_t bga;

device_t * get_storage_controller (){
    return &storage_controller;
}

device_t * get_bga(){
    return &bga;
}

uint16_t  pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t address;
    uint32_t lbus  = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}


uint32_t  pci_read_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
 
    uint32_t address = 0x80000000 | (lbus << 16) | (lslot << 11) | (lfunc << 8) | ((offset)&0xFC);
    outl(0xCF8, address);

    /* read in the data */

    return inl(0xCFC);
}


uint32_t pci_get_bar(device_t * dev, uint8_t index ){
    uint8_t offset = 0x10 + index * 4;

    uint32_t base = pci_read_long(dev->bus, dev->slot, dev->function, offset);

    if((base & 1)) 
        base &= 0xFFFFFFFC; /* I/O BAR */
    else 
        base &= (0xFFFFFFF0); /* Memory space BAR */

    return base;
}

uint16_t pci_check_vendor(uint8_t bus, uint8_t slot){
    uint16_t vendor, device;
    /* try and read the first configuration register. Since there are no */
    /* vendors that == 0xFFFF, it must be a non-existent device. */
    for(uint8_t function = 0; function<8; ++function){
        if ((vendor = pci_read_word(bus,slot,function, 0)) != 0xFFFF) {
            device = pci_read_word(bus,slot,function, 2);

            uint8_t class = (uint8_t)(pci_read_long(bus, slot, function, 0x8) >> 24); 
            uint8_t subclass = (uint8_t)(pci_read_long(bus, slot, function, 0x8) >> 16); 

            uint8_t prog_if = (uint8_t)(pci_read_long(bus, slot, function, 0x8)  >> 8);

            if(vendor == 0x1234 && device == 0x1111){
                bga.vendor_id = vendor;
                bga.device_id = device;

                bga.bus = bus;
                bga.slot = slot;
                bga.function = function;

                bga.mainclass = class;
                bga.subclass = subclass;
                bga.prog_if = prog_if;
            }

            if(class == 0x01 && subclass == 0x06 && prog_if == 0x01 ) {
                storage_controller.vendor_id = vendor;
                storage_controller.device_id = device;

                storage_controller.bus = bus;
                storage_controller.slot = slot;
                storage_controller.function = function;

                storage_controller.mainclass = class;
                storage_controller.subclass  = subclass;
                storage_controller.prog_if = prog_if;

            }
            
            printf("Got device:\n");
            printf("    class: 0x%x\n", class);
            printf("    subclass: 0x%x\n", subclass);
            printf("    vendor id: 0x%x\n", vendor);
            printf("    device id: 0x%x\n", device);
            printf("    prog IF: 0x%x\n", prog_if);
            printf("\n");
        }
    } 

    return (vendor);
}


void pci_init(){
    for(uint32_t bus=0; bus<256; ++bus){
        uint8_t device;
        for(device = 0; device < 32; device++) {
            pci_check_vendor(bus, device);
        }
    }
}

