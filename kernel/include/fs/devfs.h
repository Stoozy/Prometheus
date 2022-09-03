#pragma once

#include <fs/vfs.h>


void devfs_register_chrdev( uint64_t dev_major, uint32_t count, const char * name, struct fs * fops);

