#include "fs/tmpfs.h"
#include "fs/vfs.h"
#include "stivale2.h"
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <misc/initrd.h>
#include <string/string.h>

typedef struct {
  char name[100];
  uint64_t mode;
  uint64_t owner_id;
  uint64_t group_id;
  uint8_t size[12];
  uint8_t last_modified[12];
  uint64_t checksum;
  uint8_t type;
  uint8_t linked_file_name[100];
  uint8_t indicator;
  uint8_t version[2];
  uint8_t owner_user_name[32];
  uint8_t owner_group_name[32];
  uint64_t dev_major_number;
  uint64_t dev_minor_number;
  uint8_t filename_prefix[155];
} __attribute__((packed)) UstarFile;

static struct stivale2_module *
find_archive(struct stivale2_struct_tag_modules *modules_tag) {
  if (modules_tag == NULL)
    return NULL;

  for (int m = 0; m < modules_tag->module_count; ++m) {
    struct stivale2_module module = modules_tag->modules[m];
    if (strcmp(module.string, "INITRAMFS") == 0)
      return &modules_tag->modules[m];
  }

  return NULL;
}

static uint8_t ustar_type_to_vfs_type(uint8_t type) {
  switch (type) {
  case '0':
    return VFS_FILE;
  case '1':
    return VFS_SYMLINK;
  case '2':
    return VFS_SYMLINK;
  case '3':
    return VFS_CHARDEVICE;
  case '4':
    return VFS_BLOCKDEVICE;
  case '5':
    return VFS_DIRECTORY;
  case '6':
    return VFS_PIPE;
  default:
    return VFS_INVALID_FS;
  }
}

int oct2bin(unsigned char *str, int size) {
  int n = 0;
  unsigned char *c = str;
  while (size-- > 0) {
    n *= 8;
    n += *c - '0';
    c++;
  }
  return n;
}

static const char *get_parent_dir(const char *path) {
  char *parent = kmalloc(strlen(path));
  strcpy(parent, path);

  size_t parent_len = strlen(parent);
  if (parent[parent_len - 1] == '/')
    parent[parent_len - 1] = '\0';

  int i = parent_len;
  while (parent[i] != '/') {
    i--;
  }

  parent[i + 1] = '\0';

  return parent;
}

static int copy_file(UstarFile *file) {

  // skip .  (./etc/ becomes /etc/ )
  char *filename = file->name + 1;

  // kprintf("%s \n", filename);

  char *parent_dir = get_parent_dir(filename);
  // kprintf("Got parent dir %s\n", parent_dir);

  if (strcmp(parent_dir, "") == 0) {
    kprintf("Parent dir is empty!!\n");
    for (;;)
      ;
  }

  int filesize = oct2bin(file->size, 11);

  TmpNode *tmp_root = vfs_root.private_data;
  VFSNode *root_vn = tmp_root->vnode;

  if (ustar_type_to_vfs_type(file->type) & VFS_DIRECTORY) {
    VFSNode *parent;
    const char *parent_dir = get_parent_dir(filename);
    if (root_vn->ops->lookup(root_vn, &parent, parent_dir)) {
      kprintf("Couldn't find parent %s\n", parent_dir);
      return -1;
    }

    VFSNode *new;
    VAttr attr = {.type = VFS_DIRECTORY, .size = filesize};
    if (root_vn->ops->mkdir(parent, &new, filename, &attr)) {
      kprintf("Couldn't create %s\n", filename);
      return -1;
    }

  } else if (ustar_type_to_vfs_type(file->type) & VFS_FILE) {
    // kprintf("Regular file\n");
    VFSNode *parent;
    const char *path = get_parent_dir(filename);
    if (root_vn->ops->lookup(root_vn, &parent, path)) {
      kprintf("Couldn't find parent %s\n", path);
      return -1;
    }

    VFSNode *new;
    VAttr attr = {.type = VFS_FILE, .size = filesize};
    if (root_vn->ops->create(parent, &new, filename, &attr)) {
      kprintf("Couldn't create %s\n", filename);
      return -1;
    }

    if (root_vn->ops->write(NULL, new, ((void *)file + 512), filesize, 0) !=
        filesize) {
      kprintf("Couldn't write to file %s\n", filename);
      return -1;
    }
  }
  return 0;
}

int load_initrd(struct stivale2_struct_tag_modules *modules_tag) {
  struct stivale2_module *initrd = find_archive(modules_tag);

  if (!initrd)
    return -1;

  kprintf("Loading initrd...");
  uint8_t *ptr = (uint8_t *)initrd->begin;
  while (!memcmp(ptr + 257, "ustar", 5)) {
    UstarFile *file = (UstarFile *)ptr;
    int filesize = oct2bin(file->size, 11);

    if (copy_file(file))
      return -1;

    ptr += (((filesize + 511) / 512) + 1) * 512;
  }
  return 0;
}
