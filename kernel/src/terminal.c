#include <fs/vfs.h> #include <kmalloc.h>
#include <misc/ssfn.h>
#include <string/string.h>

#define TERM_WIDTH 80
#define TERM_HEIGHT 25

char g_term_buffer[0x1000];

static void write_char(int cx, int cy, int uc, uint32_t fg, uint32_t bg) {
  ssfn_dst.x = 8 * cx;
  ssfn_dst.y = 16 * cy + 1;

  ssfn_dst.bg = bg;
  ssfn_dst.fg = fg;

  ssfn_putc(uc);
}

static void cls() {
  // TODO:
}

void terminal_main() {
  // TODO: read tty file display chars
  // using ssfn

  memset(g_term_buffer, 0, 0x1000);
  unsigned char *buffer = kmalloc(0x1000); // 4096 byte buffer

  int cx = 0, cy = 0;

  while (true) {
    size_t len = 0;
    while (g_term_buffer[len]) {
      switch (g_term_buffer[len]) {
      case '\n':
        cy++;
        break;
      case '\t':
        cx += 4;
        break;
      default:
        write_char(cx, cy, g_term_buffer[len], 0xffffff, 0);
        if (++cx == TERM_WIDTH) {
          if (++cy == TERM_HEIGHT)
            cls();
          cx = 0;
        }
        break;
      }

      len++;
    }
    memset(g_term_buffer, 0, len);
  }
}
