#include <kernel/typedefs.h>
#include <kernel/pci.h> // For ide device_t

typedef struct IDEChannelRegisters {
   unsigned short base;  // I/O Base.
   unsigned short ctrl;  // Control Base
   unsigned short bmide; // Bus Master IDE
   unsigned char  nIEN;  // nIEN (No Interrupt);
} ide_channel_regs_t;


typedef struct ide_device {

   unsigned char  Reserved;    // 0 (Empty) or 1 (This Drive really exists).
   unsigned char  Channel;     // 0 (Primary Channel) or 1 (Secondary Channel).
   unsigned char  Drive;       // 0 (Master Drive) or 1 (Slave Drive).
   unsigned short Type;        // 0: ATA, 1:ATAPI.
   unsigned short Signature;   // Drive Signature
   unsigned short Capabilities;// Features.
   unsigned int   CommandSets; // Command Sets Supported.
   unsigned int   Size;        // Size in Sectors.
   unsigned char  Model[41];   // Model in string.

} ide_device_t;



void ide_controller_init();
 
void ide_initialize(
        uint32_t BAR0, 
        uint32_t BAR1, 
        uint32_t BAR2, 
        uint32_t BAR3, 
        uint32_t BAR4);


unsigned char ide_read(unsigned char channel, unsigned char reg);
void ide_write(unsigned char channel, unsigned char reg, unsigned char data);
void ide_read_buffer(unsigned char channel, unsigned char reg, unsigned int buffer, unsigned int quads);

unsigned char ide_polling(unsigned char channel, unsigned int advanced_check);
void ide_controller_init(device_t controller);


void ide_write_sectors(unsigned char drive, unsigned char numsects, unsigned int lba,
                        unsigned short es, unsigned int edi);

void ide_read_sectors(unsigned char drive, unsigned char numsects, unsigned int lba,
                        unsigned short es, unsigned int edi);

ide_device_t * ide_get_devices();
