#include <drivers/fb.h>
#include <drivers/video.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/typedefs.h>
#include <linux/fb.h>
#include <stivale2.h>
#include <string/string.h>

#define SSFN_CONSOLEBITMAP_TRUECOLOR /* use the special renderer for 32 bit    \
                                        truecolor packed pixels */
#include <misc/ssfn.h>

static u64 g_fb_size = 0;
static u32 *gp_framebuffer;
u32 *gp_backbuffer;
static struct stivale2_struct_tag_framebuffer *gp_fb_info;

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

void draw_fill_rect(int x, int y, int w, int h, int color) {
  draw_rect(x, y, w, h, color);

  for (u64 cy = y; cy < y + h; ++cy)
    draw_line(x, cy, x + w, cy, color);
}

void refresh_screen_proc() {
  File *fb_file = vfs_open("/dev/fb0", 0);
  struct fb_fix_screeninfo fb_fsi = fb_getfscreeninfo();
  uint8_t *backbuffer = kmalloc(fb_fsi.mmio_len);

  for (;;) {
    // seek
    fb_file->position = 0;
    vfs_read(fb_file, (void *)backbuffer, fb_fsi.mmio_len);
    memcpy((u8 *)fb_fsi.mmio_start, (void *)backbuffer, fb_fsi.mmio_len);
  }
}
