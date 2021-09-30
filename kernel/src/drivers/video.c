#include "../misc/ssfn.h"
#include "../typedefs.h"
#include "../stivale.h"
#include "video.h"
#include "../string/string.h"
#include "../kmalloc.h"

static volatile u64 g_fb_size = 0;
static volatile u32 * gp_framebuffer;
static volatile u32 * gp_backbuffer;

static struct stivale_struct * gp_vbe_info;


void screen_init(struct stivale_struct * boot_info){
    gp_vbe_info =  boot_info;

    g_fb_size = gp_vbe_info->framebuffer_width * gp_vbe_info->framebuffer_height
        * (gp_vbe_info->framebuffer_bpp/8);

    gp_framebuffer = (u32*) gp_vbe_info->framebuffer_addr;
    //gp_backbuffer  = (u32*) kmalloc(g_fb_size);

    //memset((void*)boot_info->framebuffer_addr, 0xff, g_fb_size);

} // screen_init



void draw_pixel(int x, int y, int color){

    if(x < 0 || x > gp_vbe_info->framebuffer_width  || y > gp_vbe_info->framebuffer_height || y < 0) return;

    // invalid input
    gp_framebuffer[x+y*gp_vbe_info->framebuffer_width] = color & 0xffffff;

} // draw_pixel



void draw_line(int x1, int y1, int x2, int y2, int color){
    
    // draw the line the other way
    if(x1>x2) return draw_line(x2, y1, x1, y2, color);

    if(x1 == x2) {
        for(int i=y1; i<y2; ++i){
            draw_pixel(x1, i, color);
        }
    }

    if(y1>y2) {
        y1 = -y1;
        y2 = -y2; 
        int dx = x2 - x1;
        int dy = y2 - y1;
        int d = (2 * dy) - dx;


        int cy = y1+1;
        for(int i=x1; i<x2; ++i){
            if(d <= 0) {
                draw_pixel(i+1, -(cy), color);
                dy = y2 - cy;
                int dE = 2* dy;
                d += dE;
            } else {
                draw_pixel(i+1, -(++cy), color);

                dy = y2 - cy;
                dx = x2 - i;
                int dNE = 2 * (dy - dx);
                d += dNE;
            }

        }

        return;
    } 


    int dx = x2 - x1;
    int dy = y2 - y1;
    int d = (2 * dy) - dx;


    int cy = y1+1;
    for(int i=x1; i<x2; ++i){
        if(d <= 0) {
            draw_pixel(i+1, cy, color);
            dy = y2 - cy;
            int dE = 2* dy;
            d += dE;
        } else {
            draw_pixel(i+1, ++cy, color);

            dy = y2 - cy;
            dx = x2 - i;
            int dNE = 2 * (dy - dx);
            d += dNE;
        }

    }

} // draw_line


void draw_rect(int x, int y, int w, int h, int color){
    // FIXME: check bounds

    draw_line(x, y, x+w, y, color);
    draw_line(x+w, y+1, x+w, y+h+1, color);

    draw_line(x, y+1, x, y+h+2, color);
    draw_line(x, y+h, x+w, y+h, color);

}// draw_rect

void refresh_screen_proc(){
    while(1){
        // Refresh screen here
    }
} // refresh_screen_proc


