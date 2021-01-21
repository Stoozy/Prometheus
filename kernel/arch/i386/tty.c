#include <stdbool.h>
#include <stddef.h>
//#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) {
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
	unsigned char uc = c;
    if(c == '\n') 
    {
        if(terminal_row+1 != VGA_HEIGHT){
            ++terminal_row;
        } else{

            // move each row -1
            for(size_t r = 0; r<VGA_HEIGHT; r++){
                for(size_t c = 0; c<VGA_WIDTH; c++){
                    size_t current_index = r*VGA_WIDTH+c;  // current line index
                    size_t dest_index = (r+1)*VGA_WIDTH+c; 	// next line index 
                    terminal_buffer[current_index] = terminal_buffer[dest_index];
               }
            }
            // clear last row
            for(size_t c = 0; c<VGA_WIDTH; c++){
                size_t index = VGA_HEIGHT*VGA_WIDTH+c;
                terminal_buffer[index] = vga_entry(' ', terminal_color);
            }


        };

        terminal_column = 0;
        return;
    }

    if(c == '\b'){ 
        --terminal_column;
	    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        return;
    }

	terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);

	if (++terminal_column == VGA_WIDTH) 
    {
		terminal_column = 0;
        // since it's last row, scroll up
        if(terminal_row+1 == VGA_HEIGHT){

            for(size_t r = 0; r<VGA_HEIGHT; r++){
                for(size_t c = 0; c<VGA_WIDTH; c++){
                    size_t current_index = r*VGA_WIDTH+c;  // current line index
                    size_t dest_index = (r+1)*VGA_WIDTH+c; 	// next line index 
                    terminal_buffer[current_index] = terminal_buffer[dest_index];
               }
            }

            for(size_t c = 0; c<VGA_WIDTH; c++){
                size_t index = VGA_HEIGHT*VGA_WIDTH+c;
                terminal_buffer[index] = vga_entry(' ', terminal_color);
            }

        }else{
            terminal_row++;
        }
        return;
	}
}
void terminal_panic(){
    for(size_t r=0; r<VGA_HEIGHT; r++){
        for(size_t c=0; c<VGA_WIDTH; c++){
            size_t cur_idx = r*VGA_WIDTH +c;
            terminal_buffer[cur_idx] =  vga_entry(' ', terminal_color);
        }
    }
}
void terminal_write(const char* data, size_t size) {
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data));
}

