#include <kernel/typedefs.h>

typedef struct device {
    uint8_t vendor_id;
    uint8_t device_id;
    uint8_t status;
    uint8_t command;
    uint8_t prog_if_rev_id;
    uint8_t class_data;
    uint8_t bist_header;
    uint8_t cls_lt;
} device_t; 

uint16_t  pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_check_vendor(uint8_t bus, uint8_t slot);
void pci_check_bus(uint8_t bus);
void pci_init();

