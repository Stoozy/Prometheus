#pragma once

#include <asm-generic/poll.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>
#include <unistd.h>

typedef enum vtype {
  VFS_FILE = 0x01,
  VFS_DIRECTORY = 0x02,
  VFS_CHARDEVICE = 0x03,
  VFS_BLOCKDEVICE = 0x04,
  VFS_PIPE = 0x05,
  VFS_SYMLINK = 0x06,
  VFS_MOUNTPOINT = 0x08,
  VFS_INVALID_FS = 0x09,
} vtype_t;

typedef int dev_t;
typedef uint64_t ino_t;
typedef uint32_t mode_t;

struct dirent {
  ino_t d_ino;
  ssize_t d_off;
  unsigned short int d_reclen;
  unsigned char d_type;
  char d_name[256]; /* We must not include limits.h! */
};

struct vnops;
struct vfs;

typedef struct vattr {
  vtype_t type;
  mode_t mode;
  size_t size;
  dev_t rdev; /*! device represented by file */
} VAttr;

typedef struct vnode {
  bool isroot;
  uint64_t refcnt;

  struct vfs *vfs;             /* ptr to vfs we are in */
  struct vfs *vfs_mountedhere; /* ptr to vfs (VDIR) */

  struct vnops *ops;

  enum vtype v_type; /* vnode type */
  void *private_data;

} VFSNode;

typedef struct file {
  void *magic;
  size_t refcnt;
  VFSNode *vn;
  size_t pos;
} File;

typedef struct vfs {
  SIMPLEQ_ENTRY(vfs) list;
  const struct vfsops *ops;
  VFSNode *vnodecovered; /* vnode this fs is mounted over */
  void *private_data;    /* fs-private data */
} VFS;

/*cn_nameiop flags*/

#define LOOKUP 1 // perform name lookup only
#define CREATE 2 // set up for file creation
#define DELETE 3 // set up for file deletion
#define RENAME 4 // set up for file renaming
#define OPMASK 5 // mask for operation

typedef struct componentname {
  uint32_t cn_nameiop;    /* namei operation */
  uint32_t cn_flags;      /* flags to namei */
  const char *cn_nameptr; /* pointer to looked up name */
  size_t cn_namelen;      /* length of looked up component */
  size_t cn_consume;      /* chars to consume in lookup() */
} ComponentName;

typedef struct vnops {
  int (*create)(VFSNode *dvn, VFSNode **out, struct componentname *cname,
                VAttr *attr);
  int (*getattr)(VFSNode *vn, VAttr *out);
  int (*lookup)(VFSNode *dvn, VFSNode **out, struct componentname *name);

  int (*mkdir)(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp,
               struct vattr *vap);
  int (*mknod)(struct vnode *dvp, struct vnode **vpp, struct componentname *cnp,
               struct vattr *vap);
  int (*open)(VFSNode *vn, VFSNode **out, int mode);
  int (*read)(VFSNode *vn, void *buf, size_t nbyte, off_t off);
  int (*readdir)(VFSNode *dvn, void *buf, size_t nbyte, size_t *bytesRead,
                 off_t seqno);
  int (*write)(VFSNode *vn, void *buf, size_t nbyte, off_t off);

} VNodeOps;

typedef struct vfsops {
  int (*mount)(VFS *vfs, const char *path, void *data);
  int (*root)(VFS *vfs, VFSNode **out);
  int (*vget)(VFS *vfs, VFSNode **out, ino_t inode);
} VFSOps;

int vfs_lookup(VFSNode *cwd, VFSNode **out, const char *path, uint32_t flags,
               VAttr *attr);

extern VFS vfs_root;
extern VFSNode *root_vnode;
