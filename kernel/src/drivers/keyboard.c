#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <fs/vfs.h>
#include <kprintf.h>
#include <proc/proc.h>

void handle_scan(u8 scan_code) {
  // TODO check for key release, and then write to tty
  extern Tty *gp_active_tty;
  if (gp_active_tty) {
    tty_dev_write(gp_active_tty, 1, &kbdmix[scan_code]);
  }
}
