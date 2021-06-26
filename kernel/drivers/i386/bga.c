#include <kernel/typedefs.h>
#include <drivers/bga.h>
#include <kernel/io.h>
#include <stdio.h>


#define VBE_DISPI_IOPORT_INDEX  0x01CE // write index 
#define VBE_DISPI_IOPORT_DATA   0x01CF // read data (16 bits)

#define VBE_DISPI_BANK_ADDRESS          0xA0000
#define VBE_DISPI_BANK_SIZE_KB          64

#define VBE_DISPI_INDEX_ID              0
#define VBE_DISPI_INDEX_XRES            1
#define VBE_DISPI_INDEX_YRES            2
#define VBE_DISPI_INDEX_BPP             3
#define VBE_DISPI_INDEX_ENABLE          4
#define VBE_DISPI_INDEX_BANK            5
#define VBE_DISPI_INDEX_VIRT_WIDTH      6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     7
#define VBE_DISPI_INDEX_X_OFFSET        8
#define VBE_DISPI_INDEX_Y_OFFSET        9

#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_INDEX_ENABLE          0x04
#define VBE_DISPI_NOCLEARMEM            0x81

#define VBE_DISPI_LFB_ENABLED           0

void bga_write(unsigned short IndexValue, unsigned short DataValue)
{
    outw(VBE_DISPI_IOPORT_INDEX, IndexValue);
    outw(VBE_DISPI_IOPORT_DATA, DataValue);
}
 
unsigned short bga_read(unsigned short IndexValue)
{
    outw(VBE_DISPI_IOPORT_INDEX, IndexValue);
    return inw(VBE_DISPI_IOPORT_DATA);
}
 
void bga_set_video_mode(uint32_t width, uint32_t height, uint32_t bit_depth,
        int use_linear_frame_buffer, int clear_video_mem)
{
    bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    bga_write(VBE_DISPI_INDEX_XRES, width);
    bga_write(VBE_DISPI_INDEX_YRES, height);
    bga_write(VBE_DISPI_INDEX_BPP, bit_depth);
    bga_write(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED |
        (use_linear_frame_buffer ? VBE_DISPI_LFB_ENABLED : 0) |
        (clear_video_mem ? 0 : VBE_DISPI_NOCLEARMEM));
}
 
void bga_set_bank(uint16_t bank_num)
{
    bga_write(VBE_DISPI_INDEX_BANK, bank_num);
}


uint8_t bga_available(){
    uint16_t bga_id = bga_read(0);

    printf("[BGA] Got BGA ID: 0x%x\n", bga_id);

    if(bga_id == 0xB0C5) {
        printf("[BGA] Latest version of BGA is present\n");
        return 1;
    }else if (bga_id >= 0xB0C0 && bga_id <= 0xB0C3){
        printf("[BGA] Older version of BGA found\n");
        return 1;
    }else{
        return 0;
    }
}


