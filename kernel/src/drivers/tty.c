#include "cpu/idt.h"
#include "memory/vmm.h"
#include "proc/proc.h"
#include <drivers/tty.h>
#include <fs/devfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/ringbuffer.h>
#include <libk/util.h>
#include <stdint.h>
#include <string/string.h>

struct tty *gp_active_tty;
ProcessQueue tty_wait_queue;

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
    RingBuffer *in = kmalloc(sizeof(RingBuffer));
    RingBuffer *out = kmalloc(sizeof(RingBuffer));

    rb_init(in, TTY_BUFSIZE, sizeof(uint8_t));
    rb_init(out, TTY_BUFSIZE, sizeof(uint8_t));

    g_tty_table[g_tty_counter] =
        (struct tty){.driver = driver, .ibuf = in, .obuf = out};

    return &g_tty_table[g_tty_counter++];
}

static File *file_from_tty(struct tty *tty, int minor) {
    File *file = kmalloc(sizeof(File));
    file->size = TTY_BUFSIZE;
    file->device = MKDEV(tty->driver.dev_major, minor);
    file->fs = &tty_fops;
    file->position = 0;
    file->private_data = tty;

    file->name = kmalloc(strlen("ttyX") + 1);
    memcpy(file->name, "ttyX\0", 5);
    file->name[3] = minor + '0';

    return file;
}

File *tty_open(const char *filename, int flags) {
    kprintf("[TTY]  Called open on %s\n", filename);

    int minor = filename[3] - '0';

    if (minor > MAX_TTYS || minor < 0)
        return NULL;

    kprintf("Minor is %d\n", minor);

    // first search for it
    if (g_tty_table[minor].driver.dev_major)
        return file_from_tty(&g_tty_table[minor], minor);

    struct tty *new_tty = tty_init_dev(default_tty_driver);
    kprintf("Tty not found, making a new one\n");

    gp_active_tty = new_tty;
    return file_from_tty(new_tty, minor);
}

void tty_close(struct file *file) {
    // TODO
    return;
}

size_t tty_read(struct file *file, size_t size, uint8_t *buf) {
    struct tty *tty = file->private_data;
    if (tty->driver.read)
        return tty->driver.read(tty, size, buf);

    return 0;
}

size_t tty_write(struct file *file, size_t size, uint8_t *buf) {
    struct tty *tty = file->private_data;
    if (tty) {

        if (tty->driver.write) {

            return tty->driver.write(tty, size, buf);
        } else {
            kprintf("Write not implemented :(\n");
        }
    }

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

static void echo(struct tty *tty, uint8_t val) {
    disable_irq();
    rb_push(tty->obuf, &val);
    enable_irq();
}

size_t tty_default_read(struct tty *tty, size_t size, uint8_t *buffer) {

    kprintf("[TTY]  Default read called for %d bytes\n", size);

    int s = 0;

read:
    disable_irq();

    for (; s < size; s++)
        if (!rb_pop(tty->ibuf, &buffer[s]))
            echo(tty, buffer[s]);
    enable_irq();

    if(s == 0)
        goto read;

    // should be in line disc

    return s;
}

size_t tty_default_write(struct tty *tty, size_t size, uint8_t *buffer) {
    kprintf("[TTY]  Default write called\n");
    kprintf("Data: ");

    disable_irq();
    // TODO: find a better way
    gp_active_tty = tty;

    for (int i = 0; i < size; i++) {
        if (!rb_push(tty->obuf, &buffer[i]))
            for (;;)
                kprintf("Couldn't write to buffer :(\n");

        kprintf("%c", ((char *)tty->obuf->buffer)[i]);
    }
    enable_irq();
    kprintf("\n");
    kprintf("[TTY] New length is %d\n", tty->obuf->len);


    kprintf("[TTY] Returning %ld\n", size);
    return size;
}

void tty_default_put_char(struct tty *tty, char c) {}

void tty_default_flush_chars(struct tty *tty) {
    // TODO
}

size_t tty_default_write_room(struct tty *tty) { return 0; }

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
