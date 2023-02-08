#include <linux/fb.h>
#include <unistd.h>
#include <stivale2.h>

struct fb_file {
  char name[256];
  void *data;
  size_t size;
};

void fb_init(struct stivale2_struct_tag_framebuffer *fb_info);
struct fb_fix_screeninfo fb_getfscreeninfo();
struct fb_var_screeninfo fb_getvscreeninfo();
