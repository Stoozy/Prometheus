#include <abi-bits/fcntl.h>
#include <drivers/fb.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <linux/fb.h>
#include <string/string.h>
#include <util.h>

struct file *devfs_finddir(VfsNode *dir, const char *name);
DirectoryEntry *devfs_readdir(VfsNode *dir, u32 index);
uint64_t devfs_write(struct file *file, size_t size, u8 *buffer);
uint64_t devfs_read(struct file *file, size_t size, u8 *buffer);
void devfs_close(struct file *f);
struct file *devfs_open(const char *filename, int flags);

FileSystem g_devfs = {.name = "devfs",
                      .device = 0,
                      .open = devfs_open,
                      .read = devfs_read,
                      .close = devfs_close,
                      .write = devfs_write,
                      .readdir = devfs_readdir,
                      .finddir = devfs_finddir};

#define MAX_DEVICES 256

struct devfs_entry {
  char name[256];

  char *data;
  size_t size;

  int type;
  int inode;
};

typedef struct devfs_entry DevFsRoot[MAX_DEVICES];

DevFsRoot g_devfs_root;

/*
 * mount on /dev
 * open -> allocate mem and make nodes device point to buffer (if file dne)
 * read -> simply read from device ptr
 * close -> supposed to free memory
 */

static File *devfs_entry_to_file(struct devfs_entry entry) {
  File *file = kmalloc(sizeof(File));

  file->name = kmalloc(strlen(entry.name) + 1);
  strcpy(file->name, entry.name);

  file->fs = &g_devfs;
  file->inode = entry.inode;
  file->position = 0;
  file->size = entry.size;
  file->type = entry.type;
  file->next = NULL;

  return file;
}

static size_t devfs_get_bufsize_from_name(const char *name) {
  if (starts_with(name, "fb"))
    return fb_getfscreeninfo().mmio_len;

  return 0x1000; // default buffer size
}

static struct devfs_entry devfs_create(const char *name) {
  size_t bufsize = devfs_get_bufsize_from_name(name);

  for (int i = 1; i < MAX_DEVICES; i++) {
    if (!g_devfs_root[i].inode) {
      g_devfs_root[i] = (struct devfs_entry){.name = 0,
                                             .inode = i,
                                             .type = VFS_CHARDEVICE,
                                             .data = kmalloc(bufsize),
                                             .size = bufsize};

      if (strlen(name) > 256) {
        kprintf("Name too long :(\n");
        for (;;)
          ;
      }

      strcpy(g_devfs_root[i].name, name);

      return g_devfs_root[i];
    }
  }

  kprintf("No space for any more devices :(\n");
  for (;;)
    ;
}

struct file *devfs_finddir(VfsNode *dir, const char *name) {
  (void)dir;

  if (strlen(name) > 256) {
    kprintf("Name too long\n");
    for (;;)
      ;
  }

  for (int i = 0; i < MAX_DEVICES; i++) {
    if (strcmp(name, g_devfs_root[i].name) == 0) {
      return devfs_entry_to_file(g_devfs_root[i]);
    }
  }

  return NULL;
}

DirectoryEntry *devfs_readdir(VfsNode *dir, u32 index) {
  kprintf("Devfs readdir not implemented\n");
  for (;;)
    ;
}

/* path is relative to `/dev` */
struct file *devfs_open(const char *path, int flags) {
  kprintf("[DEVFS]    called open on %s\n", path);
  struct file *file = devfs_finddir(NULL, path);

  if (!file && (flags & O_CREAT))
    return devfs_entry_to_file(devfs_create(path));

  return file;
}

uint64_t devfs_write(struct file *file, size_t size, u8 *buffer) {
  if (size > file->size) {
    kprintf("Write size is larger than actual size\n");
    return 0;
  }

  char *data_ptr = g_devfs_root[file->inode].data;
  //  kprintf("[DEVFS]  Writing to %s %s", file->name, buffer);
  memcpy(data_ptr + file->position, buffer, size);
  kprintf("[DEVFS]  Written to %s", file->name);

  return size;
}

uint64_t devfs_read(struct file *file, size_t size, u8 *buffer) {
  // kprintf("[DEVFS]  Called read on %s. File size is %d\n", file->name,
  // file->size);

  if (size > file->size)
    return 0;

  char *data = g_devfs_root[file->inode].data;
  memcpy(buffer, data + file->position, size);

  kprintf("Read %c\n", buffer);
  return size;
}

void devfs_close(struct file *f) { return; }

void devfs_init() {
  memset(g_devfs_root, 0, sizeof(DevFsRoot));
  vfs_register_fs(&g_devfs, 0);
  vfs_mount(NULL, "/dev/", "devfs", 0, NULL);
}
