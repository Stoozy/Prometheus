#include <drivers/video.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <misc/ssfn.h>
#include <stivale2.h>
#include <string/string.h>
#include <typedefs.h>

#define SSFN_CONSOLEBITMAP_TRUECOLOR

static u64 g_fb_size = 0;
static u32 *gp_framebuffer;
static u32 *gp_backbuffer;

static struct stivale2_struct_tag_framebuffer *gp_fb_info;

void screen_init(struct stivale2_struct_tag_framebuffer *fb_info) {
  gp_fb_info = fb_info;

  g_fb_size = fb_info->framebuffer_width * fb_info->framebuffer_height *
              (fb_info->framebuffer_bpp / 8);

  gp_framebuffer = (u32 *)fb_info->framebuffer_addr;
  gp_backbuffer = (u32 *)kmalloc(g_fb_size);

  draw_fill_rect(100, 100, 100, 100, 0xff0000);
  draw_fill_rect(200, 100, 100, 100, 0x00ff00);
  draw_fill_rect(300, 100, 100, 100, 0x0000ff);

  draw_line(fb_info->framebuffer_width / 2, 0, fb_info->framebuffer_width / 2,
            fb_info->framebuffer_height, 0xffffff);
  draw_line(0, fb_info->framebuffer_height / 2, fb_info->framebuffer_width,
            fb_info->framebuffer_height / 2, 0xffffff);

} // screen_init

void draw_pixel(int x, int y, int color) {

  //// invalid input
  if (x < 0 || x > gp_fb_info->framebuffer_width ||
      y > gp_fb_info->framebuffer_height || y < 0)
    return;

  gp_framebuffer[x + y * gp_fb_info->framebuffer_width] = color & 0xffffff;

} // draw_pixel

void draw_line(int x1, int y1, int x2, int y2, int color) {

  // draw the line the other way
  if (x1 > x2)
    return draw_line(x2, y1, x1, y2, color);

  if (x1 == x2) {
    for (int i = y1; i < y2; ++i) {
      draw_pixel(x1, i, color);
    }
  }

  if (y1 > y2) {
    y1 = -y1;
    y2 = -y2;
    int dx = x2 - x1;
    int dy = y2 - y1;
    int d = (2 * dy) - dx;

    int cy = y1 + 1;
    for (int i = x1; i < x2; ++i) {
      if (d <= 0) {
        draw_pixel(i + 1, -(cy), color);
        dy = y2 - cy;
        int dE = 2 * dy;
        d += dE;
      } else {
        draw_pixel(i + 1, -(++cy), color);

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

  int cy = y1 + 1;
  for (int i = x1; i < x2; ++i) {
    if (d <= 0) {
      draw_pixel(i + 1, cy, color);
      dy = y2 - cy;
      int dE = 2 * dy;
      d += dE;
    } else {
      draw_pixel(i + 1, ++cy, color);

      dy = y2 - cy;
      dx = x2 - i;
      int dNE = 2 * (dy - dx);
      d += dNE;
    }
  }

} // draw_line

void draw_rect(int x, int y, int w, int h, int color) {
  // FIXME: check bounds

  draw_line(x, y, x + w, y, color);
  draw_line(x + w, y + 1, x + w, y + h + 1, color);

  draw_line(x, y + 1, x, y + h + 2, color);
  draw_line(x, y + h, x + w, y + h, color);

} // draw_rect

// /////////////////
// @param x         x-coordinate
// @param y         y-coordinate
// @param w         width
// @param h         height
// @param color     32-bit hex value
// /////////////////
void draw_fill_rect(int x, int y, int w, int h, int color) {
  draw_rect(x, y, w, h, color);

  for (u64 cy = y; cy < y + h; ++cy)
    draw_line(x, cy, x + w, cy, color);
}

void refresh_screen_proc() {
  for (;;) {
    // memset(gp_backbuffer, 0x1f, g_fb_size);
    memcpy(gp_framebuffer, gp_backbuffer, g_fb_size);
  }
}

u32 *get_framebuffer_addr() { return gp_framebuffer; }

u64 get_framebuffer_size() { return g_fb_size; }
