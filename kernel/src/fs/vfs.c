#include <fcntl.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>
#include <util.h>

FileSystem *gp_filesystems = NULL;
VfsNode *gp_root;
VfsOpenListNode *gp_open_list;

Mountpoint *gp_mp_list;

// TODO: have a mountpoint list containing filesystems
//
// open -> consult mountpoint list and get filesystem for the given path
//  ex. open('/dev/fb0') should return devfs because of matching mount path /dev
//      open('/abc/xyz') sould return tarfs because of matching mount path /

/* check if a starts with b */

static Mountpoint *mp_from_path(const char *path) {
  int max_match_len = 0;

  Mountpoint *cmp = gp_mp_list;
  Mountpoint *matching_mp = NULL;

  if (!cmp)
    return NULL;

  for (; cmp; cmp = cmp->next) {
    if (starts_with(path, cmp->path) && strlen(cmp->path) > max_match_len) {
      matching_mp = cmp;
      max_match_len = strlen(cmp->path);
    }
    // kprintf("%s doesn't start with %s\n", path, cmp->path);
  }

  kprintf("[VFS]  File belongs to %s\n", matching_mp->fs->name);

  return matching_mp;
}
static void vfs_fs_dump(void) {
  kprintf("\n\nVFS FILESYSTEMS\n");
  FileSystem *fs = gp_filesystems;

  for (; fs; fs = fs->next)
    kprintf("%s\n", fs->name);

  kprintf("\n");
}
static void vfs_mounts_dump(void) {
  kprintf("\n\nVFS MOUNTPOINTS\n\n");
  Mountpoint *mp = gp_mp_list;

  for (; mp; mp = mp->next)
    kprintf("%s mounted at %s\n", mp->fs->name, mp->path);
}

File *vfs_open(const char *path, int flags) {
  kprintf("[VFS]  Called open on %s\n", path);

  Mountpoint *mp = mp_from_path(path);
  if (!mp) {
    kprintf("The file doesn't belong to any mounted filesystem\n");
    vfs_mounts_dump();
    for (;;)
      ;
  }

  // pass mountpoint relative path
  return mp->fs->open(path + strlen(mp->path), flags);
}

void vfs_close(File *file) {
  // TODO
  return;
  VfsOpenListNode *current_node = gp_open_list;

  while (current_node->next != NULL) {
    /* find the file */
    if (strcmp(current_node->next->vfs_node->file->name, file->name) == 0) {
      VfsOpenListNode *free_this = current_node->next;

      /* relink here */
      VfsOpenListNode *new_next = current_node->next->next;
      current_node->next = new_next;

      kfree(free_this);
    }
  }
}

ssize_t vfs_read(File *file, u8 *buffer, size_t size) {
  kprintf("[VFS]  Called read on %s for %llu bytes\n", file->name, size);

  if (file) {
    size_t bytes = file->fs->read(file, size, buffer);
    return bytes;
  }

  return -1;
}

VfsNode *vfs_node_from_path(VfsNode *parent, const char *name) {

  if (!parent)
    return NULL;

  if (strcmp(parent->file->name, name) == 0)
    return parent;

  VfsNode *node = parent->children;

  // check in-memory tree
  for (; node; node = node->next)
    if (strcmp(node->file->name, name) == 0)
      return node;

  // check underlying filesystem
  File *found_file =
      parent->file->fs->finddir(parent, name + strlen(parent->file->name));

  if (found_file) {
    kprintf("Found file %s in %s\n", name, parent->file->name);
    // update in-memory tree
    VfsNode *node = kmalloc(sizeof(VfsNode));
    node->parent = parent;
    node->file = found_file;
    node->next = NULL;

    // TODO: abstract this to another function
    if (!parent->children)
      parent->children = node;
    else {

      VfsNode *cur_child = parent->children;
      while (cur_child->next)
        cur_child = cur_child->next;
      cur_child->next = node;
    }

    return node;
  }

  // not found
  return NULL;
}

DirectoryEntry *vfs_readdir(File *file) {
  VfsNode *node = vfs_node_from_path(gp_root, file->name);

  if (node)
    return node->file->fs->readdir(node, ++file->position);

  return NULL;
}

ssize_t vfs_write(File *file, u8 *buffer, size_t size) {
  kprintf("[VFS]    Called write on %s\n", file->name);
  if (file) {
    size_t bytes = file->fs->write(file, size, buffer);
    return bytes;
  }

  return -1;
}

void vfs_register_fs(FileSystem *fs, uint64_t device_id) {
  kprintf("Registering filesystem %s\n", fs->name);

  if (gp_filesystems == NULL) {
    gp_filesystems = fs;
  } else {
    FileSystem *cur_fs = gp_filesystems;

    while (cur_fs->next != NULL)
      cur_fs = cur_fs->next;
    cur_fs->next = fs;
  }

  kprintf("Registered filesystem %s\n", fs->name);
  vfs_fs_dump();
  return;
}

void vfs_unregister_fs(FileSystem *fs) {
  /* TODO */

  return;
}

static FileSystem *fs_from_name(const char *name) {
  FileSystem *cfs = gp_filesystems;

  if (!cfs) {
    kprintf("FileSystem list is empty :(\n");
    return NULL;
  } else if (strcmp(cfs->name, name) == 0)
    return cfs;
  else {
    while (cfs) {
      kprintf("[VFS]  Found fs %s\n", cfs->name);
      if (strcmp(cfs->name, name) == 0)
        return cfs;
      cfs = cfs->next;
    }
  }

  return NULL;
}

static void vfs_add_mountpoint(FileSystem *fs, const char *path) {
  // kprintf("Mounting %s at %s\n", fs->name, path);
  Mountpoint *cmp = gp_mp_list;

  if (!cmp) {
    gp_mp_list = kmalloc(sizeof(Mountpoint));
    gp_mp_list->path = kmalloc(strlen(path) + 1);
    strcpy(gp_mp_list->path, path);

    gp_mp_list->fs = fs;
    gp_mp_list->next = NULL;
    return;
  }

  while (cmp->next)
    cmp = cmp->next;

  cmp->next = kmalloc(sizeof(Mountpoint));
  cmp = cmp->next;

  cmp->path = kmalloc(strlen(path) + 1);
  memset(cmp->path, 0, strlen(path) + 1);
  strcpy(cmp->path, path);

  cmp->fs = fs;
  cmp->next = NULL;

  kprintf("Mounted %s at %s\n", fs->name, path);

  vfs_mounts_dump();

  return;
}

int vfs_mount(const char *src, const char *dst, const char *fs_type,
              uint64_t flags, const char *data) {
  (void)src;

  VfsNode *node = vfs_node_from_path(gp_root, dst);

  if (!node) {
    kprintf("Couldn't get vfs node for %s\n", dst);
    for (;;)
      ;
  }

  FileSystem *fs = fs_from_name(fs_type);
  vfs_mounts_dump();

  if (!fs) {
    kprintf("FileSystem not found for %s\n", fs_type);
    for (;;)
      ;
  }
  node->file->fs = fs;
  node->file->type |= VFS_MOUNTPOINT;

  vfs_add_mountpoint(fs, dst);

  return 0;
}

static void _vfs_rec_dump(VfsNode *node) {
  kprintf("Found %s\n", node->file->name);

  return;
}

void vfs_dump() { _vfs_rec_dump(gp_root); }

void vfs_get_stat(const char *path, VfsNodeStat *res) {
  VfsNode *node = vfs_node_from_path(gp_root, path);

  if (node) {
    res->filesize = node->file->size;
    res->inode = node->file->inode;
    res->type = node->file->type;
  } else {
    kprintf("Path doesn't exist: %s\n", path);
    res->filesize = 0;
    res->inode = 0;
    res->type = 0;
  }

  return;
}

void vfs_init() {
  gp_root = kmalloc(sizeof(VfsNode));

  gp_root->children = NULL;

  gp_root->file = kmalloc(sizeof(File));
  gp_root->file->name = kmalloc(2);
  gp_root->file->name[0] = '/';
  gp_root->file->name[1] = '\0';
  gp_root->file->type = VFS_DIRECTORY;
  gp_root->file->fs = NULL;

  return;
}
