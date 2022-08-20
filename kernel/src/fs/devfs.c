#include <abi-bits/fcntl.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>

VfsNode *gp_devfs_root;

struct file *devfs_finddir(VfsNode *dir, const char *name);
DirectoryEntry *devfs_readdir(VfsNode *dir, u32 index);
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
  VfsNode *cnode = gp_devfs_root->children;

  kprintf("[DEVFS] Looking for %s\n", name);

  for (; cnode; cnode = cnode->next) {
    kprintf("Found %s\n", cnode->file->name);
    if (strcmp(cnode->file->name, name) == 0) {
      kprintf("Found %s\n", name);
      return cnode->file;
    }
  }

  kprintf("[DEVFS] Could not find %s\n", name);
  return NULL;
}

static void devfs_insert_node(VfsNode *node) {
  VfsNode *cnode = gp_devfs_root->children;
  node->next = NULL;
  if (cnode == NULL) // first child is non existent
    gp_devfs_root->children = node;
  else {
    while (cnode->next != NULL)
      cnode = cnode->next;

    node->next = NULL;
    cnode->next = node;
  }

  kprintf("Created node %s\n", node->file->name);
  return;
}

/* path is relative to `/dev` */
struct file *devfs_open(const char *path, int flags) {
  struct file *file = devfs_finddir(NULL, path);

  if (!file) {
    /* have to create file here */
    struct file *file = kmalloc(sizeof(File));
    memset(file, 0, sizeof(File));

    file->type = VFS_FILE;
    file->name = kmalloc(strlen(path));
    strcpy(file->name, path);

    file->fs = &g_devtmpfs;

    uint8_t *block = kmalloc(0x1000);
    memset(block, 0, 0x1000);
    file->device = (uintptr_t)block;
    file->size = 0x1000;
    file->position = 0;

    VfsNode *dev_node = kmalloc(sizeof(VfsNode));
    dev_node->file = file;

    devfs_insert_node(dev_node);

    return file;
  }

  return file;
}

u64 devfs_write(struct file *file, size_t size, u8 *buffer) {
  if (file->position > file->size)
    return 0;

  char *data_ptr = (char *)file->device;
  // kprintf("[DEVFS]  Writing to %s %s", file->name, buffer);
  memcpy(data_ptr + file->position, buffer, size);
  kprintf("[DEVFS]  Written to %s", file->name);

  return 1;
}

u64 devfs_read(struct file *file, size_t size, u8 *buffer) {
  kprintf("[DEVFS]  Called read on %s. File size is %d\n", file->name,
          file->size);
  if (file->position + size > file->size) {
    kprintf("File position + size is %x\n", file->position + size);
    kprintf("File size is  %d\n", file->size);
    return 0;
  }

  char *data_ptr = (char *)file->device;

  kprintf("Copying %u bytes\n", size);
  memcpy(buffer, data_ptr + file->position, size);
  kprintf("Read %u bytes\n", size);
  // file->position += size;

  return size;
}

void devfs_close(struct file *f) { return; }

void devfs_init(VfsNode *root_fs) {
  VfsNode *devfs_root_node = kmalloc(sizeof(VfsNode));
  devfs_root_node->file = kmalloc(sizeof(File));
  devfs_root_node->file->fs = &g_devtmpfs;
  devfs_root_node->file->type = VFS_DIRECTORY | VFS_MOUNTPOINT;
  devfs_root_node->file->name = kmalloc(256);

  memcpy(devfs_root_node->file->name, "dev", 6);
  devfs_root_node->next = NULL;

  // we know it's NULL for now from vfs_init
  // so just set it, otherwise we'd have to iterate first
  root_fs->children = devfs_root_node;

  gp_devfs_root = devfs_root_node;
}
