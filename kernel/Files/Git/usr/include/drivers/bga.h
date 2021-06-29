#pragma once

#include <kernel/typedefs.h>

uint8_t bga_available();


void bga_set_video_mode(
        uint32_t width, 
        uint32_t height, 
        uint32_t bit_depth,
        int use_linear_frame_buffer, 
        int clear_video_mem);


void bga_set_bank(uint16_t bank_num);
