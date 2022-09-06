#include "libc/abi-bits/fcntl.h"
#include "libk/kmalloc.h"
#include "libk/kprintf.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include <drivers/fb.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <libk/typedefs.h>
#include <linux/fb.h>
#include <string/string.h>

/* TODO: This only supports one framebuffer */

struct file *fb_finddir(VfsNode *dir, const char *name);
DirectoryEntry *fb_readdir(VfsNode *dir, u32 index);
uint64_t fb_write(struct file *file, size_t size, u8 *buffer);
uint64_t fb_read(struct file *file, size_t size, u8 *buffer);
void fb_close(struct file *f);
struct file *fb_open(const char *filename, int flags);
int fb_ioctl(struct file *, uint32_t request, void *arg);

FileSystem g_fbdev = {.name = "fb",
                      .open = fb_open,
                      .read = fb_read,
                      .write = fb_write,
                      .close = fb_close,
                      .readdir = NULL,
                      .finddir = NULL,
                      .ioctl = fb_ioctl};

struct fs *fbdev = &g_fbdev;

struct fb_file {
  char name[256];

  void *data;
  size_t size;
};

static struct fb_file fb;

static struct file *file_from_fb(struct fb_file fb_dev_file) {
  File *file = kmalloc(sizeof(File));
  file->name = kmalloc(strlen(fb_dev_file.name) + 1);
  strcpy(file->name, fb_dev_file.name);
  *(file->name + strlen(fb_dev_file.name)) = '\0'; // null terminate
  file->size = fb_dev_file.size;
  file->position = 0;
  file->fs = &g_fbdev;
  file->type = VFS_CHARDEVICE;
  file->inode = (uintptr_t)fb.data;

  return file;
}

static File *fb_create(const char *name) {

  struct fb_fix_screeninfo fi = fb_getfscreeninfo();
  strcpy(fb.name, name);
  fb.data = pmm_alloc_blocks(fi.mmio_len / PAGE_SIZE) + PAGING_VIRTUAL_OFFSET;
  fb.size = fi.mmio_len;

  return file_from_fb(fb);
}

struct file *fb_open(const char *filename, int flags) {
  kprintf("[FB]   opening %s\n", filename);

  if (fb.data)
    return file_from_fb(fb);
  else
    return fb_create(filename);

  return NULL;
}

uint64_t fb_write(struct file *file, size_t size, u8 *buffer) {
  kprintf("[FB]   Writing to %s\n", file->name);
  char *data = fb.data + file->position;
  memcpy(data, buffer, size);
  file->position += size;
  kprintf("[FB]   Written %d bytes to %s\n", size, file->name);
  return size;
}

uint64_t fb_read(struct file *file, size_t size, u8 *buffer) {
  char *data = fb.data + file->position;
  memcpy(buffer, data, size);
  file->position += size;
  return size;
}

void fb_close(struct file *file) {
  // TODO
  return;
}
int fb_ioctl(struct file *file, uint32_t request, void *arg) {
  kprintf("[FB]   Called ioctl\n");
  switch (request) {
  case FBIOGET_VSCREENINFO: {
    struct fb_var_screeninfo *vi = (struct fb_var_screeninfo *)arg;
    *vi = fb_getvscreeninfo();
    break;
  }
  case FBIOGET_FSCREENINFO: {
    struct fb_fix_screeninfo *fi = (struct fb_fix_screeninfo *)arg;
    *fi = fb_getfscreeninfo();
    break;
  }
  default:
    for (;;)
      kprintf("Unknown fb_ioctl request %lu\n", request);
    break;
  }

  return 0;
}

void fbdev_init() { devfs_register_chrdev(29, 1, "fb", fbdev); }
