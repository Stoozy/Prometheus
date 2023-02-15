#pragma once

#include "fs/vfs.h"
extern struct vfsops devfs_vfsops;
extern struct vnops devfs_vnops;

typedef enum device_type { BLOCKDEV, CHARDEV } DeviceType;

#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)

#define MAJOR(dev) ((unsigned int)((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int)((dev)&MINORMASK))
#define MKDEV(ma, mi) (((ma) << MINORBITS) | (mi))

typedef struct chardev {
  VNodeOps fs;
  dev_t rdev;
  char *filename;
  void *private_data;
  size_t size;
} CharacterDevice;

typedef struct dev {
  DeviceType type;
  union {
    CharacterDevice cdev;
  };
} Device;

#define MAX_DEVICES 255

extern VFSNode *dev_root;
int devfs_init();