#include <kernel/typedefs.h>
#include <kernel/pci.h>

#include <drivers/ahci.h>

#include <stdio.h>

static int check_type(HBA_PORT * port);

void ahci_init(device_t * dev){
    printf("Initializing achi controller \n");

    HBA_MEM * ABAR = (HBA_MEM*)pci_get_bar(dev, 5);

    printf("Device ID 0x%x \n", dev->device_id); printf("Vendor ID 0x%x \n", dev->vendor_id);
    printf("ABAR at 0x%x\n", ABAR);

    probe_ports(ABAR);
}


void probe_ports(HBA_MEM * abar){
    uint32_t pi = abar->port_impl;
    for(int i=0; i<32; ++i){
        if(pi & 1){
            int device_type = check_type(&abar->ports[i]);
            switch(device_type){
                case AHCI_DEV_NULL:
                    printf("Not supported.\n");
                    break;
                case AHCI_DEV_SATA:
                    printf("SATA device found.\n");
                    break;
                case AHCI_DEV_SATAPI:
                    printf("SATAPI device found.\n");
                    break;
            }
        }
        pi >>= 1;
    }
}

static int check_type(HBA_PORT * port){
    switch(port->signature){
        case SATA_SIG_ATA:
            return AHCI_DEV_SATA;    
        case SATA_SIG_ATAPI:
            return AHCI_DEV_SATAPI;
        default:
            return AHCI_DEV_NULL;
    }
}
