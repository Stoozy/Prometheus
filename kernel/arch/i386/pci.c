#include <kernel/typedefs.h>
#include <kernel/pci.h>

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


uint16_t pci_check_vendor(uint8_t bus, uint8_t slot){
    uint16_t vendor, device;
    /* try and read the first configuration register. Since there are no */
    /* vendors that == 0xFFFF, it must be a non-existent device. */
    for(uint8_t function = 0; function<8; ++function){
        if ((vendor = pci_read(bus,slot,function, 0)) != 0xFFFF) {
            device = pci_read(bus,slot,function, 2);
            uint16_t class = pci_read(bus, slot, function, 10); 
            printf("Got device:\n");
            printf("    vendor id: 0x%x\n", vendor);
            printf("    device id: 0x%x\n", device);
            printf("    class: 0x%x\n", (uint8_t) class );
            printf("    subclass: 0x%x\n", (uint8_t) class << 8);
            printf("\n");
            Sleep(1000);
        }
    } 

    return (vendor);
}



void pci_check_bus(uint8_t bus){
    uint8_t device;

    for(device = 0; device < 32; device++) {
        uint16_t vendorID = pci_check_vendor(bus, device);
    }
}


void pci_init(){
    for(uint32_t bus=0; bus<256; ++bus){
        pci_check_bus(bus);
    }
}

