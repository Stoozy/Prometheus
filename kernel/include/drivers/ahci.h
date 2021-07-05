#pragma once

#include <kernel/pci.h>

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	    0x96690101	// Port multiplier

#define AHCI_DEV_NULL 0
#define AHCI_DEV_SATA 1
#define AHCI_DEV_SEMB 2
#define AHCI_DEV_PM 3
#define AHCI_DEV_SATAPI 4

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3


typedef volatile struct tagHBA_PORT
{
	uint32_t cmd_list_base;		// 0x00, command list base address, 1K-byte aligned
	uint32_t cmd_list_base_upper;		// 0x04, command list base address upper 32 bits
	uint32_t fis_base;		// 0x08, FIS base address, 256-byte aligned
	uint32_t fis_base_upper;		// 0x0C, FIS base address upper 32 bits
	uint32_t interrupt_status;		// 0x10, interrupt status
	uint32_t interrupt_enable;		// 0x14, interrupt enable
	uint32_t cmd_and_status;		// 0x18, command and status
	uint32_t rsv0;		// 0x1C, Reserved
	uint32_t task_file_data;		// 0x20, task file data
	uint32_t signature;		// 0x24, signature
	uint32_t sata_status;		// 0x28, SATA status (SCR0:SStatus)
	uint32_t sata_ctl;		// 0x2C, SATA control (SCR2:SControl)
	uint32_t sata_err;		// 0x30, SATA error (SCR1:SError)
	uint32_t sata_active;		// 0x34, SATA active (SCR3:SActive)
	uint32_t cmd_issue;		// 0x38, command issue
	uint32_t sata_notif;		// 0x3C, SATA notification (SCR4:SNotification)
	uint32_t fis_switch_ctl;		// 0x40, FIS-based switch control
	uint32_t rsv1[11];	// 0x44 ~ 0x6F, Reserved
	uint32_t vendor[4];	// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;



typedef volatile struct tagHBA_MEM {
	// 0x00 - 0x2B, Generic Host Control
	uint32_t host_capabilities;		// 0x00, Host capability
	uint32_t global_host_ctl;		// 0x04, Global host control
	uint32_t interrupt_status;		// 0x08, Interrupt status
	uint32_t port_impl;		// 0x0C, Port implemented
	uint32_t version;		// 0x10, Version
	uint32_t ccc_ctl;	// 0x14, Command completion coalescing control
	uint32_t ccc_pts;	// 0x18, Command completion coalescing ports
	uint32_t em_loc;		// 0x1C, Enclosure management location
	uint32_t em_ctl;		// 0x20, Enclosure management control
	uint32_t host_capabilities_ext;		// 0x24, Host capabilities extended
	uint32_t bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	uint8_t  rsv[0xA0-0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	uint8_t  vendor[0x100-0xA0];

	// 0x100 - 0x10FF, Port control registers
	HBA_PORT	ports[1];	// 1 ~ 32
} HBA_MEM;


void ahci_init(device_t * dev);
void probe_ports(HBA_MEM * abar);

