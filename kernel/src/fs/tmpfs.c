#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <string/string.h>

#include <sys/queue.h>

static int tmpfs_mount(VFS *vfs, const char *path, void *data) { return -1; }
static int tmpfs_root(VFS *vfs, VFSNode **out) { return -1; }
static int tmpfs_vget(VFS *vfs, VFSNode **out, ino_t inode) { return -1; }

VFSOps tmpfs_vfsops = {
    .mount = tmpfs_mount, .root = tmpfs_root, .vget = tmpfs_vget};

static int tmpfs_create(VFSNode *dvn, VFSNode **out, const char *name,
                        VAttr *attr) {
  return -1;
}
static int tmpfs_getattr(VFSNode *dvn, VAttr *out) { return -1; }
static int tmpfs_open(VFSNode *vn, VFSNode **out, int mode) { return -1; }
static int tmpfs_read(VFSNode *vn, void *buf, size_t nbyte, off_t off) {
  return -1;
}
static int tmpfs_readdir(VFSNode *dvn, void *buf, size_t nbyte,
                         size_t *bytesRead, off_t seqno) {
  return -1;
}
int tmpfs_write(VFSNode *vn, void *buf, size_t nbyte, off_t off) { return -1; }

VNodeOps tmpfs_vnops = {.create = tmpfs_create,
                        .open = tmpfs_open,
                        .read = tmpfs_read,
                        .write = tmpfs_write,
                        .readdir = tmpfs_readdir};
