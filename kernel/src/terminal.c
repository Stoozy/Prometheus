#include "abi-bits/fcntl.h"
#include "drivers/tty.h"
#include "libk/kprintf.h"
#include <drivers/fb.h>
#include <drivers/tty.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <linux/fb.h>
#include <misc/ssfn.h>
#include <string/string.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

#define TERM_WIDTH 200
#define TERM_HEIGHT 100

#define TERM_BUFSIZE 0x1000

#define WHITE 0xffffff
#define BLACK 0x0

extern unsigned char iso_font[256][16];

static struct fb_var_screeninfo fb_vsi;
static uint32_t *backbuffer;

static uint8_t g_term_buffer[TERM_BUFSIZE];
typedef struct tchar {
  uint8_t c;
  uint32_t fg;
  uint32_t bg;
} __attribute__((packed)) tchar_t;

tchar_t term_screen[TERM_WIDTH][TERM_HEIGHT];

static void fb_putpixel(int x, int y, uint32_t color) {
  backbuffer[y * fb_vsi.xres + x] = color;
}

static void draw_char(uint8_t block[FONT_WIDTH], int x, int y, uint32_t fg,
                      uint32_t bg) {
  for (int i = 0; i < FONT_HEIGHT; i++) {
    for (int j = 0; j < FONT_WIDTH; j++) {
      if (block[i] >> j & 1)
        fb_putpixel(x + j, y + i, fg);
    }
  }
}

static void redraw_screen() {
  for (int r = 0; r < TERM_HEIGHT; r++) {
    for (int c = 0; c < TERM_WIDTH; c++) {
      tchar_t tchar = term_screen[c][r];
      draw_char(iso_font[tchar.c], r * FONT_WIDTH, c * FONT_HEIGHT, tchar.fg,
                tchar.bg);
    }
  }
}

static void cls() {
  // TODO:
}

void terminal_main() {
  // TODO: read tty file display chars
  // using ssfn

  memset(g_term_buffer, 0, TERM_BUFSIZE);
  File *fb_file = vfs_open("/dev/fb0", O_RDWR);
  backbuffer = (uint32_t *)fb_file->inode;
  fb_vsi = fb_getvscreeninfo();

  // File *tty_file = vfs_open("/dev/tty0", O_RDONLY);

  extern struct tty *gp_active_tty;
  int cx = 0, cy = 0;
  for (;;) {
    kprintf("Running terminal ... \n");
    if (!gp_active_tty)
      continue;

    // size_t br = vfs_read(tty_file, &g_term_buffer[0], TERM_BUFSIZE);

    // size_t br = gp_active_tty->driver.read(gp_active_tty, TERM_BUFSIZE,
    //&g_term_buffer[0]);
    // kprintf("[TERM] Got %d bytes\n", br);
    // for (int i = 0; i < br; i++) {
    // kprintf(" %c ", g_term_buffer[i]);
    //}
    kprintf("\n");

    size_t len = 0;
    while (len < TERM_BUFSIZE) {
      if (!rb_pop(gp_active_tty->obuf, &g_term_buffer[len]))
        break;

      switch (g_term_buffer[len]) {
      case '\n':
        cy++;
        cx = 0;
        break;
      case '\t':
        cx += 4;
        break;
      default:
        term_screen[cy][cx] =
            (tchar_t){.c = g_term_buffer[len], .fg = WHITE, .bg = BLACK};
        cx++;
      };

      ++len;
    }

    redraw_screen();
  }
}
