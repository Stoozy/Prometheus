#include <kernel/typedefs.h>
#include <kernel/pci.h>
#include <stdio.h>

void ahci_init(device_t * dev){
    printf("Initializing achi controller \n");

    printf("Class 0x%x \n", dev->mainclass);
    printf("Subclass 0x%x \n", dev->subclass);
}
