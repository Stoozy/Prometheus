#include "syscalls.h"
#include "../proc/tasking.h"
#include "../fs/vfs.h"

char * __env = {0};
/* pointer to array of char * strings that define the current environment variables */
char **environ = &__env; 

void sys_exit(){
    _kill(); 
}

int sys_close(int file){
    // TODO 
    return -1;
}

int sys_execve(char *name, char **argv, char **env){
    // TODO 
    int f = vfs_open(name, 0);
    return -1;
}


int sys_fork(){
    return -1; 
}
