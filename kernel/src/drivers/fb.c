#include "abi-bits/fcntl.h"
#include "fs/tmpfs.h"
#include "libk/util.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include <drivers/fb.h>
#include <fs/devfs.h>
#include <fs/vfs.h>
#include <memory/slab.h>
#include <libk/kprintf.h>
#include <linux/fb.h>
#include <string/string.h>

struct fb_var_screeninfo fb0_vsi;
struct fb_fix_screeninfo fb0_fsi;

VFSNode *fb_vnode;
uint8_t *gp_backbuffer;

static void fb_init_vsi(struct stivale2_struct_tag_framebuffer *fb_info,
                        struct fb_var_screeninfo *fb0_vsi) {

  fb0_vsi->bits_per_pixel = fb_info->framebuffer_bpp;
  fb0_vsi->xres = fb_info->framebuffer_width;
  fb0_vsi->yres = fb_info->framebuffer_height;

  fb0_vsi->xres_virtual = fb_info->framebuffer_width;
  fb0_vsi->yres_virtual = fb_info->framebuffer_height;
  fb0_vsi->grayscale = 0;

  fb0_vsi->red.length = fb_info->red_mask_size;
  fb0_vsi->red.offset = fb_info->red_mask_shift;

  fb0_vsi->blue.length = fb_info->blue_mask_size;
  fb0_vsi->blue.offset = fb_info->blue_mask_shift;

  fb0_vsi->green.length = fb_info->green_mask_size;
  fb0_vsi->green.offset = fb_info->green_mask_shift;
  return;
}

static void fb_init_fsi(struct stivale2_struct_tag_framebuffer *fb_info,
                        struct fb_fix_screeninfo *fb0_fsi) {

  fb0_fsi->line_length = fb_info->framebuffer_pitch;
  fb0_fsi->smem_len = fb0_fsi->mmio_len =
      fb_info->framebuffer_height * fb_info->framebuffer_pitch;
  fb0_fsi->mmio_start = fb_info->framebuffer_addr;
  fb0_fsi->visual = FB_VISUAL_TRUECOLOR;

  return;
}

struct fb_fix_screeninfo fb_getfscreeninfo() { return fb0_fsi; }
struct fb_var_screeninfo fb_getvscreeninfo() { return fb0_vsi; }

// /* TODO: This only supports one framebuffer */

#define FB_MAJOR 29
int fbops_getattr(VFSNode *vn, VAttr *out) {
  kprintf("fbops_getattr()");
  for (;;)
    ;
}

int fbops_open(File *file, VFSNode *vn, int mode) {
  if (vn != fb_vnode)
    return -1;
  file->vn = vn;
  file->pos = 0;
  file->refcnt = 1;

  return 0;
}

ssize_t fbops_read(File *file, VFSNode *vn, void *buf, size_t nbyte,
                   off_t off) {
  // kprintf("fbops_read()\n");
  TmpNode *tnode = vn->private_data;
  void *framebuffer = tnode->dev.cdev.private_data + off;
  if (nbyte + off > tnode->dev.cdev.size) {
    kprintf("Reading outside of file");
    return -1;
  }

  memcpy(buf, framebuffer, nbyte);
  return nbyte;
}

ssize_t fbops_write(File *file, VFSNode *vn, void *buf, size_t nbyte,
                    off_t off) {
  TmpNode *fbnode = vn->private_data;
  void *framebuffer = fbnode->dev.cdev.private_data;
  if (nbyte + off > fbnode->dev.cdev.size) {
    kprintf("Trying to write outide of framebuffer\n");
    return -1;
  }

  memcpy(fbnode->dev.cdev.private_data + off, buf, nbyte);
  return nbyte;
}

int fbops_ioctl(VFSNode *vn, uint64_t request, void *arg, int fflag) {
  kprintf("fbops_ioctl()");

  switch (request) {
  case FBIOGET_VSCREENINFO: {
    kprintf("[FB] getting vscreeninfo\n");
    struct fb_var_screeninfo *vi = (struct fb_var_screeninfo *)arg;
    *vi = fb_getvscreeninfo();
    break;
  }
  case FBIOGET_FSCREENINFO: {
    kprintf("[FB] getting fscreeninfo\n");
    struct fb_fix_screeninfo *fi = (struct fb_fix_screeninfo *)arg;
    *fi = fb_getfscreeninfo();
    kprintf("FRAMEBUFFER MMIO LEN IS %d\n", fi->mmio_len);
    break;
  }
  default:
    kprintf("Unknown fb_ioctl request %lu\n", request);
    for (;;)
      ;
    break;
  }

  return 0;
}

VNodeOps fb_vnops = {.open = fbops_open,
                     .read = fbops_read,
                     .write = fbops_write,
                     .ioctl = fbops_ioctl};

void fb_proc() {
  struct fb_fix_screeninfo fb_fsi = fb_getfscreeninfo();
  File *fb_file = vfs_open("/dev/fb0", O_RDWR);
  if (fb_file == NULL)
    kprintf("File not found");
  else
    kprintf("File found");

  memset(gp_backbuffer, 255, fb_fsi.mmio_len);

  for (;;) {
    // seek
    fb_file->pos = 0;
    asm("cli");
    fb_file->vn->ops->read(fb_file, fb_file->vn, gp_backbuffer, fb_fsi.mmio_len,
                           0);
    asm("sti");

    memcpy((uint8_t *)fb_fsi.mmio_start, (void *)gp_backbuffer,
           fb_fsi.mmio_len);
  }
}

int fb_init(struct stivale2_struct_tag_framebuffer *fb_info) {
  /* init framebuffer structures */
  fb_init_vsi(fb_info, &fb0_vsi);
  fb_init_fsi(fb_info, &fb0_fsi);

  VAttr attr = {.type = VFS_CHARDEVICE,
                .size = fb0_fsi.mmio_len,
                .rdev = MKDEV(FB_MAJOR, 0)};
  kprintf("Creating /dev/fb0\n");
  dev_root->ops->create(dev_root, &fb_vnode, "/dev/fb0", &attr);
  TmpNode *tnode = fb_vnode->private_data;
  tnode->dev.cdev.private_data =
      PAGING_VIRTUAL_OFFSET +
      pmm_alloc_blocks(DIV_ROUND_UP(fb0_fsi.mmio_len, PAGE_SIZE));

  gp_backbuffer = tnode->dev.cdev.private_data;

  kprintf("backbuffer is @ 0x%p", gp_backbuffer);
  // set current nodes fs operations to framebuffer driver ops
  fb_vnode->ops = &fb_vnops;

  return 0;
}
