#pragma once
#include <kernel/typedefs.h>


typedef struct device {
    uint16_t vendor_id;
    uint16_t device_id;

    uint8_t bus;
    uint8_t slot;
    uint8_t function;

    uint8_t mainclass;
    uint8_t subclass;
    uint8_t prog_if;

} device_t; 

device_t * get_storage_controller();
device_t get_bga();

uint16_t pci_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_read_long(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pci_get_bar(device_t dev, uint8_t index);
uint16_t pci_check_vendor(uint8_t bus, uint8_t slot);
void pci_check_bus(uint8_t bus);
void pci_init();


