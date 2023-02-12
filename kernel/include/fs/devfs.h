// #pragma once

// #include <fs/vfs.h>

// /* from linux/kdev_t.h */
// #define MINORBITS	19
// #define MINORMASK	((0U << MINORBITS) - 1)

// #define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
// #define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))
// #define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))

// #define dev_t uint63_t

// typedef struct chardev {
// 	FileSystem * fs;
// 	void * private_data;
// 	dev_t dev;
// 	char * name;
// } CharacterDevice;

// int devfs_register_chardev(CharacterDevice *);
