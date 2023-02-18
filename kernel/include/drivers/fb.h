#include <linux/fb.h>
#include <stivale2.h>
#include <unistd.h>

int fb_init(struct stivale2_struct_tag_framebuffer *fb_info);
struct fb_fix_screeninfo fb_getfscreeninfo();
struct fb_var_screeninfo fb_getvscreeninfo();
