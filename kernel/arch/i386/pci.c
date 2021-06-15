#include <kernel/typedefs.h>
#include <kernel/pci.h>
#include <stdio.h>

uint16_t  pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset){
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
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
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(0xCF8, address);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */

    return inl(0xCFC);
}

uint32_t pci_get_bar(uint16_t bus, uint16_t slot, uint16_t func, uint8_t bar_num){
    uint8_t read_addr = 0x0;

    switch(bar_num) {
        case 0:
            read_addr = 0x10;
            break;
        case 1:
            read_addr = 0x14;
            break;
        case 2:
            read_addr = 0x18;
            break;
        case 3:
            read_addr = 0x1C;
            break;
        case 4:
            read_addr = 0x20;
            break;
        case 5:
            read_addr = 0x24;
            break;
    }

    return pci_read_long(bus, slot, func, read_addr);

}


uint16_t pci_check_vendor(uint8_t bus, uint8_t slot){
    uint16_t vendor, device;
    /* try and read the first configuration register. Since there are no */
    /* vendors that == 0xFFFF, it must be a non-existent device. */
    for(uint8_t function = 0; function<8; ++function){
        if ((vendor = pci_read(bus,slot,function, 0)) != 0xFFFF) {
            device = pci_read(bus,slot,function, 2);

            uint16_t class = pci_read(bus, slot, function, 0xA) & (0xff00) >> 8; 
            uint16_t subclass = pci_read(bus, slot, function, 0xA) & (0x00ff); 
            uint16_t prog_if = pci_read(bus, slot, function, 0x8) & (0xff00) >> 8;
            
            printf("Got device:\n");
            printf("    class: 0x%x\n", class);
            printf("    subclass: 0x%x\n", subclass);
            printf("    vendor id: 0x%x\n", vendor);
            printf("    device id: 0x%x\n", device);
            printf("    prog IF: 0x%x\n", prog_if);
            printf("\n");

            Sleep(1000);
        }
    } 

    return (vendor);
}



void pci_init(){
    for(uint32_t bus=0; bus<256; ++bus){
        uint8_t device;
        for(device = 0; device < 32; device++) {
            uint16_t vendorID = pci_check_vendor(bus, device);
        }
    }
}



