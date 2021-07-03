#include <kernel/typedefs.h>
#include <fs/fs.h>


int register_fs(fs_type_t * type)
{
    // should call mount here
    printf("A %s filesystem has been registered", type->name);
}

int unregister_fs(fs_type_t * type)
{

}
 
