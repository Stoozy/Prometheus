#include <fs/vfs.h>
#include <libk/kprintf.h>

VFS vfs_root;

int vfs_lookup(VFSNode *cwd, VFSNode **out, const char *path, uint32_t flags,
               VAttr *attr) {

  return -1;
}
