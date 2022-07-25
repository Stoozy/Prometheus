#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string.h>

VfsNode *gp_devfs_root;

struct file *devfs_finddir(VfsNode *dir, const char *name);
struct dirent *devfs_readdir(VfsNode *dir, u32 index);
u64 devfs_write(struct file *file, size_t size, u8 *buffer);
u64 devfs_read(struct file *file, size_t size, u8 *buffer);
void devfs_close(struct file *f);
struct file *devfs_open(const char *filename, int flags);

FileSystem g_devtmpfs = {.name = "devfs\0",
                         .device = 0,
                         .open = devfs_open,
                         .read = devfs_read,
                         .close = devfs_close,
                         .write = devfs_write,
                         .readdir = NULL,
                         .finddir = devfs_finddir};

/*
 * mount on /dev
 * open -> allocate mem and make nodes device point to buffer (if file dne)
 * read -> simply read from device ptr
 * close -> supposed to free memory
 */

struct file *devfs_finddir(VfsNode *dir, const char *name) {
  // not using dir
  VfsNode *current_node = gp_devfs_root->children;
  if (current_node) {
    if (strcmp(current_node->file->name, name) == 0)
      return current_node->file;

  } else
    return NULL;

  while (current_node->next) {
    if (strcmp(current_node->file->name, name) == 0)
      return current_node->file;
  }

  return NULL;
}

/* path is relative to `/dev` */
struct file *devfs_open(const char *path, int flags) {
  kprintf("Called devfs open\n");
  struct file *file = devfs_finddir(NULL, path);

  if (!file) {
    /* have to create file here */

    struct file *file = kmalloc(sizeof(File));
    memset(file, 0, sizeof(File));

    file->name = (char *)path;
    file->fs = &g_devtmpfs;

    file->device = (u64)kmalloc(0x10000);
    file->size = 0x10000;

    return file;
  }

  return file;
}

u64 devfs_write(struct file *file, size_t size, u8 *buffer) {
  if (file->position > file->size + size) {
    return 0;
  }

  char *data_ptr = (char *)file->device;
  kprintf("[DEVFS]  Writing to %s %s", file->name, buffer);
  memcpy(data_ptr + file->position, buffer, size);

  return 1;
}
u64 devfs_read(struct file *file, size_t size, u8 *buffer) {
  if (file->position + size > file->size)
    return 0;

  char *data_ptr = (char *)file->device;
  memcpy(buffer, data_ptr + file->position, size);
  while (!buffer[0]) {
    memcpy(buffer, data_ptr + file->position, size);
  }
  return 1;
}
void devfs_close(struct file *f) { return; }

void devfs_init(VfsNode *root_fs) {
  VfsNode *devfs_root_node = kmalloc(sizeof(VfsNode));
  devfs_root_node->file = kmalloc(sizeof(File));
  devfs_root_node->file->fs = &g_devtmpfs;
  devfs_root_node->type = VFS_DIRECTORY | VFS_MOUNTPOINT;
  devfs_root_node->file->name = kmalloc(256);

  memcpy(devfs_root_node->file->name, "dev", 6);
  devfs_root_node->next = NULL;

  // we know it's NULL for now from vfs_init
  // so just set it, otherwise we'd have to iterate first
  root_fs->children = devfs_root_node;

  gp_devfs_root = devfs_root_node;
}
