#include "vfs.h"
#include "../kmalloc.h"
#include "../kprintf.h"

static FileSystem * g_filesystems[VFS_MAX_DEVICES] = {0}; 

static u16 get_device_index_from_id(u8 * device){
    u64 index =  (((device[0] - 'a') * 10) + (device[1] - 48));
    return (u16)index;
}

static void vfs_test_get_device_from_index(){
    u8 * dev = kmalloc(sizeof(u8) * 4);
    dev[0] = 'a';
    dev[1] = '0';
    dev[2] = '\0';

    u16 index = get_device_index_from_id(dev);
    kprintf("Testing a0 index : %d\n", index);

    dev[0] = 'b';
    dev[1] = '8';
    dev[2] = '\0';
    index = get_device_index_from_id(dev);
    kprintf("Testing b8 index : %d\n", index);
}

static u8 * get_device_from_global_path(const char * path ){
    u8 * device = kmalloc(sizeof(char) * 4); 
    device[0] = path[0];
    device[1] = path[1];
    device[2] = '\0';

    return device;
}



void vfs_register_fs(FileSystem  * new_fs, u16 device_id){
    char device[3];
    device[0] =  'a' + (device_id / 10);
    device[1] =  '0'+device_id % 10;
    device[2] = '\0';

    kprintf("[VFS]  Registering %s on index %d\n", device, device_id);

    if(device_id < VFS_MAX_DEVICES && new_fs)
        g_filesystems[device_id] = new_fs;

    return;
}

void vfs_unregister_fs(FileSystem * fs){
    for(u16 fs_idx=0; fs_idx < VFS_MAX_DEVICES; ++fs_idx){
        if(g_filesystems[fs_idx] == fs){
            // free some memory
            kfree(fs->name);
            kfree(fs);

            // empty fs entry
            g_filesystems[fs_idx] = 0;
        }
    }
}


u64 vfs_read(FILE * file, u64 size, u8 * buffer){
    u16 fs_idx = file->device; 

    if(g_filesystems[fs_idx]){
        u64 bytes_read  = g_filesystems[fs_idx]->read(file, size, buffer);
        return bytes_read;
    }

    // zero bytes read
    return 0;

}


u64 vfs_write(FILE * file, u64 size, u8 * buffer){
    u16 fs_idx = file->device;

    if(g_filesystems[fs_idx]){
        u64 bytes_written  = g_filesystems[fs_idx]->write(file, size, buffer);
        return bytes_written;
    }

    // zero bytes written
    return 0;

}


FILE * vfs_open(const char *filename, int flags){
    if(filename){
        if(filename[2] == ':'){
            u8 * device_id = get_device_from_global_path(filename);
            u16 fs_idx = get_device_index_from_id(device_id); 

            // skip device identifier 
            // e.g. a0:/foo/bar => /foo/bar
            filename += 3;
            if(g_filesystems[fs_idx]){
                kprintf("Got fs_idx %d\n", fs_idx);
                FILE * file = g_filesystems[fs_idx]->open(filename, flags);
                if(file == NULL)
                    return NULL;

                kprintf("[VFS]  Opened: %s\n", file->name);
                kprintf("File inode %llx\n", file->inode);
                file->device = fs_idx;
                return file;
            }
        }
    }

    FILE * f = kmalloc(sizeof(FILE));
    f->mode = VFS_INVALID_FS;
    return f;
}

void vfs_close(FILE file){
    // TODO
}

