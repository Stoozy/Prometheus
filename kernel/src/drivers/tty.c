#include "kmalloc.h"
#include "kprintf.h"
#include <drivers/tty.h>
#include <fs/devfs.h>
#include <string/string.h>
#include <util.h>

struct tty g_tty_table[MAX_TTYS];
static uint8_t g_tty_counter;

// could be as many as total number of ttys
struct tty_ldisc g_ldiscs[MAX_TTYS];
struct tty_driver g_tty_drivers[MAX_TTYS];
static struct tty_driver default_tty_driver;

File *tty_open(const char *filename, int flags);
void tty_close(struct file *);
size_t tty_read(struct file *, size_t, uint8_t *buf);
size_t tty_write(struct file *, size_t, uint8_t *buf);
int tty_ioctl(struct file *, uint32_t request, void *arg);
int tty_poll(struct file *, struct pollfd *, int timeout);

FileSystem tty_fops = {.open = tty_open,
                       .close = tty_close,
                       .read = tty_read,
                       .write = tty_write,
                       .ioctl = tty_ioctl,
                       .poll = tty_poll};

/* initializes new device */

struct tty *tty_init_dev(struct tty_driver driver) {
  struct tty new_tty = (struct tty){.driver = driver};

  g_tty_table[g_tty_counter] = new_tty;
  ++g_tty_counter;

  return &g_tty_table[g_tty_counter - 1];
}

File *tty_open(const char *filename, int flags) {
  kprintf("[TTY]  Called open on %s\n", filename);

  struct tty *new_tty = tty_init_dev(default_tty_driver);

  // ttyN
  int minor = filename[4] - '0';

  if (minor > MAX_TTYS)
    return NULL;

  File *file = kmalloc(sizeof(File));
  file->size = TTY_BUFSIZE;
  file->device = MKDEV(new_tty->driver.dev_major, minor);
  file->fs = &tty_fops;
  file->position = 0;
  file->private_data = new_tty;

  file->name = kmalloc(strlen(filename));
  strcpy(file->name, filename);

  return file;
}

void tty_close(struct file *file) {
  // TODO
  return;
}

size_t tty_read(struct file *file, size_t size, uint8_t *buf) {
  struct tty *tty = file->private_data;
  if (tty)
    return tty->driver.read(tty, size, buf);
  return 0;
}

size_t tty_write(struct file *file, size_t size, uint8_t *buf) {
  struct tty *tty = file->private_data;
  if (tty)
    return tty->driver.write(tty, size, buf);
  return 0;
}

int tty_ioctl(struct file *file, uint32_t request, void *arg) {
  struct tty *tty = file->private_data;
  if (tty)
    return tty->driver.ioctl(tty, request, arg);
  return 0;
}

int tty_poll(struct file *file, struct pollfd *pfd, int timeout) {
  struct tty *tty = file->private_data;
  if (tty)
    return tty->driver.poll(tty, pfd, timeout);

  return 0;
}

size_t tty_default_read(struct tty *tty, size_t size, uint8_t *buffer) {
  // TODO: notify on full buffer

  // just use the primary buffer
  __asm__("cli");
  if (tty->flip.primary_idx > TTY_BUFSIZE)
    tty->flip.primary_idx = 0;

  int pos = tty->flip.primary_idx;
  memcpy(buffer, &tty->flip.primary[pos], TTY_BUFSIZE - pos);

  __asm__("sti");

  return 0;
}

size_t tty_default_write(struct tty *tty, size_t size, uint8_t *buffer) {
  // TODO: notify on full buffer

  // just use the primary buffer
  __asm__("cli");
  if (tty->flip.primary_idx > TTY_BUFSIZE)
    tty->flip.primary_idx = 0;

  int pos = tty->flip.primary_idx;
  memcpy(&tty->flip.primary[pos], buffer, TTY_BUFSIZE - pos);
  tty->flip.primary_idx += size;

  __asm__("sti");

  return 0;
}

void tty_default_put_char(struct tty *tty, char c) {

  // just use the primary buffer

  __asm__("cli");
  if (tty->flip.primary_idx > TTY_BUFSIZE)
    tty->flip.primary_idx = 0;

  tty->flip.primary[tty->flip.primary_idx] = c;
  ++tty->flip.primary_idx;

  __asm__("sti");
}

void tty_default_flush_chars(struct tty *tty) {
  // TODO
}

size_t tty_default_write_room(struct tty *tty) {
  return TTY_BUFSIZE - tty->flip.primary_idx;
}

int tty_default_set_termios(struct tty *tty, struct termios tios) {
  tty->tios = tios;
  return 0;
}

int tty_default_set_ldisc(struct tty *tty, struct tty_ldisc ldisc) {
  tty->ldisc = ldisc;
  return 0;
}

int tty_default_ioctl(struct tty *tty, uint64_t request, void *arg) {
  // TODO
  return 0;
}

void tty_init() {
  default_tty_driver =
      (struct tty_driver){.tty_table = &g_tty_table[0],
                          .driver_name = "tty",
                          .name = "tty",
                          .dev_major = 5,
                          .num_devices = MAX_TTYS,
                          .read = tty_default_read,
                          .write = tty_default_write,
                          .flush_chars = tty_default_flush_chars,
                          .write_room = tty_default_write_room,
                          .set_termios = tty_default_set_termios,
                          .set_ldisc = tty_default_set_ldisc,
                          .ioctl = tty_default_ioctl};

  devfs_register_chrdev(TTY_MAJOR, MAX_TTYS, "tty", &tty_fops);
}
