#pragma once

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
  VFS_INVALID_FS = 0x09,
} vtype_t;

typedef int dev_t;
typedef uint64_t ino_t;
typedef uint32_t mode_t;

struct vnops;
struct vfs;

typedef struct directory_entry {
  ino_t d_ino;
  off_t d_off;
  uint16_t d_reclen;
  vtype_t d_type;
  char d_name[256];
} DirectoryEntry;

typedef struct vattr {
  vtype_t type;
  mode_t mode;
  size_t size;
  dev_t rdev; /*! device represented by file */
} VAttr;

typedef struct vfs_node_stat {
  enum vtype type;
  ino_t inode;
  size_t filesize;
  dev_t rdev;
} __attribute__((packed)) VFSNodeStat;

typedef struct vnode {
  bool isroot;
  uint64_t refcnt;

  struct vfs *vfs;             /* ptr to vfs we are in */
  struct vfs *vfs_mountedhere; /* ptr to vfs (VDIR) */

  struct vnops *ops;

  VFSNodeStat stat;
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

struct componentname {
  uint32_t cn_nameiop; /* namei operation */
  uint32_t cn_flags;   /* flags to namei */

  const char *cn_nameptr; /* pointer to looked up name */

  size_t cn_namelen; /* length of looked up component */
  size_t cn_consume; /* chars to consume in lookup() */
};

typedef struct vnops {
  int (*create)(VFSNode *dvn, VFSNode **out, const char *name, VAttr *attr);
  int (*getattr)(VFSNode *vn, VAttr *out);
  int (*lookup)(VFSNode *dvn, VFSNode **out, const char *name);

  int (*mkdir)(struct vnode *dvp, struct vnode **vpp, const char *name,
               struct vattr *vap);
  int (*mknod)(struct vnode *dvp, struct vnode **vpp, const char *name,
               struct vattr *vap);
  int (*open)(File *file, VFSNode *vn, int mode);
  int (*readdir)(VFSNode *dvn, void *buf, size_t nbyte, size_t *bytesRead,
                 off_t seqno);
  int (*symlink)(struct vnode *dvp, struct vnode **vpp, char *source,
                 struct vattr *vap, char *target);

  ssize_t (*read)(File *file, VFSNode *vn, void *buf, size_t nbyte, off_t off);
  ssize_t (*write)(File *file, VFSNode *vn, void *buf, size_t nbyte, off_t off);
  int (*ioctl)(VFSNode *vp, uint64_t, void *data, int fflag);
  int (*poll)(VFSNode *vp, int events);

} VNodeOps;

typedef struct vfsops {
  int (*mount)(VFS *vfs, const char *path, void *data);
  int (*root)(VFS *vfs, VFSNode **out);
  int (*vget)(VFS *vfs, VFSNode **out, ino_t inode);
} VFSOps;

struct nameidata {
  /*
   * Arguments to namei.
   */
  struct vnode *ni_atdir;     /* startup dir, cwd if null */
  struct pathbuf *ni_pathbuf; /* pathname container */
  char *ni_pnbuf;             /* extra pathname buffer ref (XXX) */
  /*
   * Internal starting state. (But see notes.)
   */
  struct vnode *ni_rootdir;  /* logical root directory */
  struct vnode *ni_erootdir; /* emulation root directory */
  /*
   * Results from namei.
   */
  struct vnode *ni_vp;  /* vnode of result */
  struct vnode *ni_dvp; /* vnode of intermediate directory */
  /*
   * Internal current state.
   */
  size_t ni_pathlen;       /* remaining chars in path */
  const char *ni_next;     /* next location in pathname */
  unsigned int ni_loopcnt; /* count of symlinks encountered */
  /*
   * Lookup parameters: this structure describes the subset of
   * information from the nameidata structure that is passed
   * through the VOP interface.
   */
  struct componentname ni_cnd;
};

int vfs_lookup(VFSNode *cwd, VFSNode **out, const char *path, uint32_t flags,
               VAttr *attr);

extern VFS vfs_root;
extern VFSNode *root_vnode;

File *vfs_open(const char *name, int flags);
int vfs_mkdir(const char *path, mode_t mode);
int vfs_stat(const char *path, VFSNodeStat *);
int vfs_close(File *);
