#include <kernel/typedefs.h>
#include <kernel/io.h>
#include <stdbool.h>
#include <stdio.h>

#define DRIVE_SELECT_PORT 	0x1F6
#define SECTOR_COUNT_PORT 	0x1F2
#define LBA_LO_PORT 		0x1F3
#define LBA_MID_PORT		0x1F4
#define LBA_HI_PORT			0x1F5
#define IDENTIFY_CMD		0xEC
#define STATUS_CMD_REG		0x1F7

#define STATUS_BSY 0x80
#define STATUS_RDY 0x40
#define STATUS_DRQ 0x08
#define STATUS_DF 0x20
#define STATUS_ERR 0x01

bool irq_fired = false;

void ata_irq(){
    printf("ATA IRQ Fired!\n");
    irq_fired = true;
}

void wait_for_irq(){
    // Do nothing while waiting
    while(!irq_fired);

    // reset when done
    irq_fired = false;
    return;
}

void ATA_WAIT_BSY(){
    while(inb(STATUS_CMD_REG) & STATUS_BSY){
        if(inb(STATUS_CMD_REG) & (1<<5)){
            printf("Drive fault error.\n");
        }
        asm("nop");
    }
}
void ATA_WAIT_RDY(){
    while(!(inb(0x1F7)&STATUS_RDY));
}

/*
 * Check if drive is valid
 *
 */
bool ATA_IDENTIFY(uint8_t drive){
    outb(DRIVE_SELECT_PORT, drive);
    outb(SECTOR_COUNT_PORT, 1);
    outb(LBA_LO_PORT, 0);
    outb(LBA_MID_PORT, 0);
    outb(LBA_HI_PORT, 0);
    outb(STATUS_CMD_REG, IDENTIFY_CMD);

    uint8_t status = inb(0x1F7);

    if(status == 0) {
        printf("Drive does not exist");
        return false;
    }else{
        ATA_WAIT_BSY();
    }

    uint8_t lba_mid = inb(LBA_MID_PORT);
    uint8_t lba_hi = inb(LBA_HI_PORT);

    if(lba_mid != 0 || lba_hi != 0){
        printf("Not an ATA drive.");
        return false;
    }

    printf("Disk status (decimal) %d\n", status);
    return !(status == 0);
}

/*
 * Read Disk Sectors
 * Params: target will be filled with the data read
 *
 */
void read_sectors(uint16_t * target, uint8_t drive, uint32_t LBA, uint8_t sector_count){ 
    uint8_t slavebit = 0;
    if(drive == 0xA0){
        drive = 0xE0;
    }else {
        slavebit = 1;
        drive = 0xF0;
    } // assume it's a slave drive

    
    outb(DRIVE_SELECT_PORT, drive | 0x40 | slavebit << 4 | ((LBA >> 24) & 0x0F));
    outb(0x1F1, 0);
    outb(SECTOR_COUNT_PORT, sector_count);
    outb(LBA_LO_PORT, (uint8_t)LBA);
    outb(LBA_MID_PORT, (uint8_t)(LBA >> 8));
    outb(LBA_HI_PORT, (uint8_t)(LBA >> 16));
    outb(STATUS_CMD_REG, 0x20);     // Read command
    
    ATA_WAIT_RDY();
    ATA_WAIT_BSY();

    for(int i=0; i<sector_count;i++){
        wait_for_irq();
        for(int j=0; j<256; j++)
            target[j] = inw(0x1F0);
        target+=256;
    }

    return;
}

/*
 * Write to disk drive
 *
 * data is the data to be written
 */
void write_sectors(uint16_t * data, uint8_t sector_count, uint32_t LBA){

    outb(DRIVE_SELECT_PORT, 0xE0 | ((LBA >> 24) & 0x0F));
    outb(0x1F1, 0);
    outb(SECTOR_COUNT_PORT, sector_count);
    outb(LBA_LO_PORT, LBA);
    outb(LBA_MID_PORT, LBA >> 8);
    outb(LBA_HI_PORT, LBA >> 16);
    outb(DRIVE_SELECT_PORT, 0x40);  // magic
    outb(STATUS_CMD_REG, 0x30);     // Write command
    
    for(int i=0; i<sector_count; i++){
        ATA_WAIT_BSY();
        ATA_WAIT_RDY();
        for(int j=0; j<256; j++)
            outw(0x1F0, data[j]);
        data += 256;
    }

    return;
}

