#include "fs/devfs.h"
#include <abi-bits/fcntl.h>
#include <drivers/fb.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/util.h>
#include <linux/fb.h>
#include <string/string.h>

#define MAX_DEVICES 256

/* table of device drivers */
static CharacterDevice *devices[MAX_DEVICES];
static int g_device_counter = 0;

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

int devfs_register_chardev(CharacterDevice *chardev) {
  if (g_device_counter > MAX_DEVICES)
    return -1;

  devices[g_device_counter++] = chardev;
  kprintf("[DEVFS] Registered %s at %d\n", chardev->name, g_device_counter - 1);
  return 0;
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

  for (int i = 0; i < g_device_counter; i++) {
    if (devices[i]) {
      kprintf("Checking against %s\n", devices[i]->name);
      if (strcmp(path, devices[i]->name) == 0)
        return devices[i]->fs->open(path, flags);
    }
  }

  for (;;)
    ;
  return NULL;
}

void devfs_init() {
  memset(devices, 0, MAX_DEVICES * sizeof(uintptr_t));
  vfs_register_fs(&g_devfs, 0);
  vfs_mount(NULL, "/dev/", "devfs", 0, NULL);
}
