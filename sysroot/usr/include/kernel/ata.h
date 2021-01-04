#include <kernel/typedefs.h>
#include <kernel/io.h>

#define DRIVE_SELECT_PORT 	0x1F6
#define SECTOR_COUNT_PORT 	0x1F2
#define LBA_LO_PORT 		0x1F3
#define LBA_MID_PORT		0x1F4
#define LBA_HI_PORT			0x1F5
#define IDENTIFY_PORT		0xEC
#define CMD_IO_PORT		    0x1F7



void read_sectors(uint16_t * target, uint8_t drive, uint32_t LBA, uint8_t sector);
void write_sectors(uint16_t * data, uint8_t sector, uint32_t LBA);
bool ATA_IDENTIFY( uint8_t drive);
void ATA_WAIT_BSY();
void ATA_WAIT_RDY();

