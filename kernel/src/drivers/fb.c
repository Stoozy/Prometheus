// #include <drivers/fb.h>
// #include <fs/vfs.h>
// #include <libk/kmalloc.h>
// #include <libk/kprintf.h>
// #include <linux/fb.h>
// #include <string/string.h>

// struct fb_var_screeninfo fb0_vsi;
// struct fb_fix_screeninfo fb0_fsi;

// static void fb_init_vsi(struct stivale2_struct_tag_framebuffer *fb_info,
//                         struct fb_var_screeninfo *fb0_vsi) {

//   fb0_vsi->bits_per_pixel = fb_info->framebuffer_bpp;
//   fb0_vsi->xres = fb_info->framebuffer_width;
//   fb0_vsi->yres = fb_info->framebuffer_height;

//   fb0_vsi->xres_virtual = fb_info->framebuffer_width;
//   fb0_vsi->yres_virtual = fb_info->framebuffer_height;
//   fb0_vsi->grayscale = 0;

//   fb0_vsi->red.length = fb_info->red_mask_size;
//   fb0_vsi->red.offset = fb_info->red_mask_shift;

//   fb0_vsi->blue.length = fb_info->blue_mask_size;
//   fb0_vsi->blue.offset = fb_info->blue_mask_shift;

//   fb0_vsi->green.length = fb_info->green_mask_size;
//   fb0_vsi->green.offset = fb_info->green_mask_shift;
//   return;
// }

// static void fb_init_fsi(struct stivale2_struct_tag_framebuffer *fb_info,
//                         struct fb_fix_screeninfo *fb0_fsi) {

//   fb0_fsi->line_length = fb_info->framebuffer_pitch;
//   fb0_fsi->mmio_len = fb_info->framebuffer_height * fb_info->framebuffer_pitch;
//   fb0_fsi->mmio_start = fb_info->framebuffer_addr;
//   fb0_fsi->visual = FB_VISUAL_TRUECOLOR;

//   return;
// }

// struct fb_fix_screeninfo fb_getfscreeninfo() {
//   return fb0_fsi;
// }
// struct fb_var_screeninfo fb_getvscreeninfo() {
//   return fb0_vsi;
// }

// void fb_init(struct stivale2_struct_tag_framebuffer *fb_info) {
//   fb_init_vsi(fb_info, &fb0_vsi);
//   fb_init_fsi(fb_info, &fb0_fsi);

//   extern void fbdev_init();
//   fbdev_init();

//   /* File *fb_dev = vfs_open("/dev/fb0", 0); */

//   // discards small buffer created by devfs and assigns a new one
//   // that is of the proper size
//   /* void *file_buf = kmalloc(fb0_fsi.mmio_len); */
//   /* memset(file_buf, 0, fb0_fsi.mmio_len); */
//   /* fb_dev->device = (uintptr_t)file_buf; */
//   /* fb_dev->size = fb0_fsi.mmio_len; */
//   /* fb_dev->position = 0; */

//   // test write to fb file
//   /* uint8_t *test_buf = kmalloc(fb0_fsi.mmio_len); */
//   /* memset(test_buf, 0x28, fb0_fsi.mmio_len); */
//   /* vfs_write(fb_dev, test_buf, fb0_fsi.mmio_len); */

//   return;
// }
