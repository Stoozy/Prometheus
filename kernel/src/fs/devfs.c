#include <abi-bits/fcntl.h>
#include <drivers/fb.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/util.h>
#include <linux/fb.h>
#include <string/string.h>

#define TTY_MAJOR 5
#define VIDEO_MAJOR 29

struct file *devfs_finddir(VfsNode *dir, const char *name);
DirectoryEntry *devfs_readdir(VfsNode *dir, u32 index);
uint64_t devfs_write(struct file *file, size_t size, u8 *buffer);
uint64_t devfs_read(struct file *file, size_t size, u8 *buffer);
void devfs_close(struct file *f);
struct file *devfs_open(const char *filename, int flags);

FileSystem g_devfs = {.name = "devfs",
                      .open = devfs_open,
                      .read = NULL,
                      .close = NULL,
                      .write = NULL,
                      .readdir = devfs_readdir,
                      .finddir = devfs_finddir};

#define MAX_DEVICES 256

/* table of device drivers */
volatile struct fs *drivers[MAX_DEVICES];

void devfs_register_chrdev(uint64_t dev_major, uint32_t count, const char *name,
                           struct fs *fops) {
  (void)name;
  drivers[dev_major] = fops;
  // kprintf("[DEVFS]  Registered %s device driver\n", name);
  return;
}

/*
 * mount on /dev
 * open -> allocate mem and make nodes device point to buffer (if file dne)
 * read -> simply read from device ptr
 * close -> supposed to free memory
 */

static size_t devfs_get_bufsize_from_name(const char *name) {
  if (starts_with(name, "fb"))
    return fb_getfscreeninfo().mmio_len;

  return 0x1000; // default buffer size
}

struct file *devfs_finddir(VfsNode *dir, const char *name) {
  (void)dir;
  return NULL;
}

DirectoryEntry *devfs_readdir(VfsNode *dir, u32 index) {
  kprintf("Devfs readdir not implemented\n");
  for (;;)
    ;
}

/* path is relative to `/dev` */
struct file *devfs_open(const char *path, int flags) {
  kprintf("[DEVFS] Opening %s\n", path);
  if (starts_with(path, "tty"))
    return drivers[TTY_MAJOR]->open(path, flags);
  else if (starts_with(path, "fb")) {
    kprintf("Fb open at %x\n", drivers[VIDEO_MAJOR]->open);
    return drivers[VIDEO_MAJOR]->open(path, flags);
  }

  kprintf("Couldn't find driver for  %s\n", path);

  return NULL;
}

void devfs_init() {
  memset(drivers, 0, MAX_DEVICES * sizeof(uintptr_t));
  vfs_register_fs(&g_devfs, 0);
  vfs_mount(NULL, "/dev/", "devfs", 0, NULL);
}
