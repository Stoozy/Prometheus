#pragma once

#include "../typedefs.h"

void screen_init();
void draw_pixel(int X, int Y, int RGB);
void draw_rect(int X, int Y, int Width, int Height, int RGB);
void draw_line(int X1, int Y1, int X2, int Y2, int RGB);
void draw_fill_rect(int x, int y, int w, int h, int color);

u32 * get_framebuffer_addr();
u64 get_framebuffer_size();

void refresh_screen_proc();

