#include <fcntl.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>

FileSystem *gp_filesystems;
VfsNode *gp_root;
VfsOpenListNode *gp_open_list;

/* check if a starts with b */
static bool starts_with(const char *a, char *b) {
  size_t alen = strlen(a);
  size_t blen = strlen(b);

  if (alen < blen) {
    return false;
  }

  int i = 0;
  while (i < blen) {
    if (a[i] != b[i])
      return false;
    i++;
  }

  return true;
}

static VfsNode *vfs_node_from_path(VfsNode *parent, const char *path) {

  // kprintf("[VFS]  Getting node from path %s\n", path);
  // kprintf("[VFS]  Parent is %s\n", parent->file->name);

  if (strcmp("/", path) == 0)
    return gp_root;

  if (strcmp("..", path) == 0)
    return parent->parent;

  if (strcmp(".", path) == 0)
    return parent;

  size_t len = strlen(path);
  size_t parent_len = strlen(parent->file->name);

  /* Iterating vfs nodes children */
  if (parent->children) {
    kprintf("Root has children\n");
    VfsNode *current_child = parent->children;
    kprintf("Childs name is %s\n", current_child->file->name);

    size_t cur_child_path_len = strlen(current_child->file->name);

    if (starts_with(&path[parent_len], current_child->file->name) &&
        strcmp(path, current_child->file->name) != 0) {
      kprintf("Relative path is  is %s\n", &path[parent_len]);
      kprintf("Found dir %s\n ", current_child->file->name);
      return vfs_node_from_path(current_child,
                                &path[parent_len + cur_child_path_len + 1]);
      // return current_child;
    } else if (strcmp(current_child->file->name, path) == 0) {
      return current_child;
    }

    while (current_child->next) {
      if (starts_with(&path[parent_len], current_child->file->name) &&
          strcmp(path, current_child->file->name) != 0) {
        kprintf("Found dir %s\n ", current_child->file->name);
        return vfs_node_from_path(current_child,
                                  &path[parent_len + cur_child_path_len + 1]);
        // return current_child;
      } else if (strcmp(current_child->file->name, path) == 0) {
        return current_child;
      }

      current_child = current_child->next;
    }
  } else {
    // child does not exist
    // create file
    File *file = parent->file->fs->open(path, 0);
    if (file) {
      VfsNode *node = kmalloc(sizeof(VfsNode));
      node->file = file;
      node->file->type = VFS_FILE;
      return node;
    }
  }

  File *file = parent->file->fs->finddir(parent, &path[1]);

  if (file) {
    VfsNode *new_node = kmalloc(sizeof(VfsNode));
    new_node->file = file;
    new_node->parent = parent;
    new_node->children = NULL;
    new_node->next = NULL;
    new_node->file->type = VFS_FILE;

    return new_node;
  }

  /* TODO:
   *
   * iterate the underlying fs
   * to see if file actually exists
   * otherwise, simply return null
   */

  return NULL;
}

File *vfs_open(const char *filename, int flags) {
  kprintf("[VFS]  Called open on %s\n", filename);

  VfsNode *node = vfs_node_from_path(gp_root, filename);

  if (node)
    return node->file;

  kprintf("[VFS] Node not found :(; while opening %s \n", filename);

  return NULL;
}

void vfs_close(File *file) {
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

ssize_t vfs_read(File *file, u8 *buffer, size_t off, size_t size) {
  kprintf("[VFS]  Called read on %s\n", file->name);
  if (file) {
    size_t bytes = file->fs->read(file, size, buffer);
    file->position += size;
    return bytes;
  }

  return -1;
}

DirectoryEntry *vfs_readdir(File *file) {
  VfsNode *node = vfs_node_from_path(gp_root, file->name);

  if (node)
    return node->file->fs->readdir(node, ++file->position);

  return NULL;
}

ssize_t vfs_write(File *file, u8 *buffer, size_t off, size_t size) {

  if (file) {
    file->position += off;
    size_t bytes = file->fs->write(file, size, buffer);
    return bytes;
  }

  return -1;
}

void vfs_register_fs(FileSystem *fs, uint64_t device_id) {
  if (gp_filesystems == NULL) {
    gp_filesystems = fs;
  } else {
    FileSystem *cur_fs = gp_filesystems;

    while (cur_fs->next != NULL)
      cur_fs = cur_fs->next;
    cur_fs->next = fs;
  }

  return;
}

void vfs_unregister_fs(FileSystem *fs) {
  /* TODO */

  return;
}

static FileSystem *fs_from_path(const char *path) {
  FileSystem *fs;

  /* TODO */

  return fs;
}

bool vfs_mount(const char *src, const char *dst) {
  /* trying to mount root */
  if (dst[0] == '/' && dst[1] == 0 && gp_root != NULL)
    return false;

  FileSystem *fs = fs_from_path(src);
  VfsNode *node = vfs_node_from_path(gp_root, dst);

  if (node) {
    node->file->type = VFS_MOUNTPOINT;
    node->file = NULL;
  } else {
    /* trying to mount on non-existent dir */
    return false;
  }

  return true;
}

static void _vfs_rec_dump(VfsNode *node) {
  kprintf("Found %s\n", node->file->name);

  if (node->file->type == VFS_DIRECTORY || node->file->type == VFS_MOUNTPOINT) {
    kprintf("Is a directory. Iterating directory...\n");
    if (node->children)
      _vfs_rec_dump(node->children);
    else
      kprintf("No children\n");
  }

  if (node->next) {
    kprintf("Checking next item in directory\n");
    _vfs_rec_dump(node->next);
  }

  kprintf("Done iterating dir\n");

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

void vfs_init(FileSystem *root_fs) {
  gp_root = kmalloc(sizeof(VfsNode));

  gp_root->parent = NULL;

  /* no need to iterate fs yet */
  gp_root->children = NULL;

  gp_root->file = kmalloc(sizeof(File));
  gp_root->file->name = kmalloc(2);
  gp_root->file->name[0] = '/';
  gp_root->file->name[1] = '\0';
  gp_root->file->type = VFS_DIRECTORY | VFS_MOUNTPOINT;

  gp_root->file->fs = root_fs;
  gp_root->file->position = 0;
  gp_root->file->device = root_fs->device;
  gp_root->parent = gp_root;

  vfs_dump();
  return;
}
