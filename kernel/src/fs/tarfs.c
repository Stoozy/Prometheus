#include "kprintf.h"
#include <dirent.h>
#include <stdarg.h>

#include <config.h>
#include <fs/tarfs.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>

struct file *ustar_finddir(VfsNode *dir, const char *name);
DirectoryEntry *ustar_readdir(VfsNode *dir, u32 index);
uint64_t ustar_write(struct file *file, size_t size, u8 *buffer);
uint64_t ustar_read(struct file *file, size_t size, u8 *buffer);
void ustar_close(struct file *f);
struct file *ustar_open(const char *filename, int flags);

FileSystem g_tarfs = {.name = "ustar\0",
                      .device = 0,
                      .open = ustar_open,
                      .read = ustar_read,
                      .close = ustar_close,
                      .write = ustar_write,
                      .readdir = ustar_readdir,
                      .finddir = ustar_finddir};

static u8 *g_archive;

static u32 oct2bin(unsigned char *str, int size) {
  int n = 0;
  unsigned char *c = str;
  while (size-- > 0) {
    n *= 8;
    n += *c - '0';
    c++;
  }
  return n;
}

static u32 ustar_decode_filesize(UstarFile *file) {
  return oct2bin(file->size, 11);
}

static u8 ustar_type_to_vfs_type(u8 type) {
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

// return len if it does start with
uint64_t starts_with(const char *a, const char *b) {
  size_t alen = strlen(a);
  size_t blen = strlen(b);

  if (alen < blen)
    return 0;

  if (blen == 0)
    return 0;

  int i = 0;
  while (i < blen) {
    if (a[i] != b[i])
      return 0;
    i++;
  }

  return i;
}

bool ends_with_slash(const char *str) {
  int len = strlen(str);
  if (str[len - 1] == '\0' && str[len - 2] == '/')
    return true;
  return false;
}

char *next_token(const char *str, char sep) {
  int i = 0;
  while (i < strlen(str)) {
    if (str[i] == sep) {
      char *new = kmalloc(i + 1);
      memcpy(new, str, i);
      new[i] = '\0';
      return new;
    }

    i++;
  }
  return NULL;
}

bool str_in_list(char **list, int llen, char *str) {
  for (int i = 0; i < llen; i++)
    if (list[i] && strcmp(list[i], str) == 0)
      return true;
  return false;
}

static DirectoryEntry *_ustar_read_dir_entries(unsigned char *archive,
                                               const char *dir_path,
                                               uint32_t index) {
  return NULL;
}

UstarFile *ustar_search(unsigned char *archive, const char *filename) {
  unsigned char *ptr = archive;

  while (!memcmp(ptr + 257, "ustar", 5)) {
    int filesize = oct2bin(ptr + 0x7c, 11);

    // kprintf("Found %s\n", ptr);
    if (!memcmp(ptr, filename, strlen(filename) + 1)) {
      UstarFile *file = (UstarFile *)ptr;
      return file;
    }
    ptr += (((filesize + 511) / 512) + 1) * 512;
  }

  return 0;
}

static u64 round_to_512_bytes(u64 bytes) {
  if (bytes % 512 == 0)
    return bytes;

  u64 padding = bytes % 512;

  return bytes + (512 - padding);
}

struct file *ustar_open(const char *filename, int flags) {

  UstarFile *tar_file = ustar_search(g_archive, filename);

  if (tar_file) {
    File *file = kmalloc(sizeof(File));
    file->size = ustar_decode_filesize(tar_file);
    file->name = (char *)filename;
    file->inode = (u64)((void *)tar_file);
    file->fs = &g_tarfs;
    return file;
  }

  return NULL;
}

void ustar_close(struct file *f) {
  // TODO
}

uint64_t ustar_read(struct file *file, size_t size, u8 *buffer) {

  kprintf("[TARFS] Called read\n");

  UstarFile *tar_fp = (UstarFile *)file->inode;
  char *sof = ((char *)(tar_fp)) + 512;
  char *begin = sof + file->position;
  kprintf("[TARFS] File data: %s\n", sof);
  if (tar_fp) {
    kprintf("[TARFS] Valid file\n");
    kprintf("Copying data over to %x (%llu bytes)\n", buffer, size);
    memcpy(buffer, begin, size);
    return size;
  }

  return 0;
}

uint64_t ustar_write(struct file *file, size_t size, u8 *buffer) {

  kprintf("ustar_write() is not implemented\n");
  for (;;)
    ;
  return 0;
}

DirectoryEntry *ustar_readdir(VfsNode *dir, u32 index) {

  // kprintf("[TARFS] Readdir on %s \n", dir->file->name);
  /* make sure node is actually a directory */
  if (!(dir->file->type & VFS_DIRECTORY)) {
    kprintf("File is not a directory!\n");
    kprintf("Type is %d\n", dir->file->type);
    return NULL;
  }

  unsigned char *ptr = g_archive;
  int i = 0;
  // kprintf("Reading entries of %s\n", dir_path);

  char **entry_list = kmalloc(sizeof(char *) * index);

  int inode = 0;
  while (!memcmp(ptr + 257, "ustar", 5)) {
    int filesize = oct2bin(ptr + 0x7c, 11);
    UstarFile *file = (UstarFile *)ptr;
    inode++;

    char *token = next_token(file->name, '/');
    // kprintf("Got token %s\n", token);

    if (!str_in_list(entry_list, index, token)) {
      // kprintf("%s not in entry list\n", token);
      // kprintf("Adding it to list\n");
      entry_list[i] = token;
      if (i == index) {
        DirectoryEntry *entry = kmalloc(sizeof(DirectoryEntry));
        memcpy(entry->name, file->name, 128);
        entry->ino = inode;
        entry->type = VFS_DIRECTORY;
        return entry;
      }

      i++;
      ptr += (((filesize + 511) / 512) + 1) * 512;
    } else {
      // kprintf("Found in entry list, skipping ... \n");
      ptr += (((filesize + 511) / 512) + 1) * 512;
      continue;
    }
  }

  return NULL;
}

struct file *ustar_finddir(VfsNode *dir, const char *name) {

  kprintf("[TARFS]    Finding %s in current directory %s\n", name,
          dir->file->name);

  UstarFile *file = ustar_search(g_archive, name);

  if (file) {
    kprintf("[TARFS]    Found %s\n", file->name);
    struct file *vfs_file = kmalloc(sizeof(struct file));
    vfs_file->name = file->name;
    vfs_file->inode = (u64)((void *)file);
    vfs_file->size = ustar_decode_filesize(file);
    vfs_file->position = 0;
    vfs_file->device = 0;
    vfs_file->next = NULL;

    vfs_file->fs = &g_tarfs;

    return vfs_file;
  }

  return NULL;
}

void tarfs_init(u8 *archive) {
  g_archive = archive;

  vfs_register_fs(&g_tarfs, 0);
  vfs_mount(NULL, "/", "ustar", 0, 0);
}
