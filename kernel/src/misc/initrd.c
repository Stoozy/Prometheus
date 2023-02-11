#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include <libk/kprintf.h>
#include <misc/initrd.h>
#include <string/string.h>

static void *find_archive(struct stivale2_struct_tag_modules *modules_tag) {
  if (modules_tag == NULL)
    return NULL;

  for (int m = 0; m < modules_tag->module_count; ++m) {
    struct stivale2_module module = modules_tag->modules[m];
    if (strcmp(module.string, "INITRAMFS") == 0)
      return (void *)module.begin;
  }

  return NULL;
}

int load_initrd(struct stivale2_struct_tag_modules *modules_tag) {
  void *archive = find_archive(modules_tag);

  if (!archive)
    return -1;

  vfs_root.ops = &tmpfs_vfsops;

  if (vfs_root.ops->mount(&vfs_root, NULL, NULL)) {
    kprintf("Couldn't mount tmpfs\n");
    for (;;)
      ;
  }

  return 0;
}
