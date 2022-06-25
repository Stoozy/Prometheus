#include <bits/ensure.h>
#include <mlibc/debug.hpp>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/thread-entry.hpp>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>

namespace mlibc {

const int SYS_EXIT          = 0;
const int SYS_OPEN          = 1;
const int SYS_CLOSE         = 2;
const int SYS_READ          = 3;
const int SYS_WRITE         = 4;
const int SYS_LOG_LIBC      = 5;
const int SYS_VM_MAP        = 6;
const int SYS_SEEK          = 7;


void sys_libc_log(const char *message) {
    register int syscall asm("rsi") = SYS_LOG_LIBC;
    register const char * msg  asm("r8") = message;

    register int r15 asm("r15");
    asm("syscall" 
            : "=r"(r15)  
            : "r" (syscall), "r"(msg) 
            : "rcx", "r11", "memory");

}

void sys_libc_panic() {
    mlibc::infoLogger() << "\e[31mmlibc: panic!" << frg::endlog;
    asm volatile ("syscall" :
            : "a"(12), "D"(1)
            : "rcx", "r11", "rdx");
}

int sys_tcb_set(void *pointer) {
   
    sys_libc_log("[mlibc]   Calling sys_tcb_set\n");
    return -1;
}

int sys_anon_allocate(size_t size, void **pointer) {

    int errno = sys_vm_map(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS, -1, 0, pointer);

    return errno;
}

int sys_anon_free(void *pointer, size_t size) {
    // TODO
    sys_libc_log("[mlibc]   Calling sys_anon_free\n");
    return 0;
}

#ifndef MLIBC_BUILDING_RTDL
void sys_exit(int status) {
    // TODO
    sys_libc_log("[mlibc]   Calling sys_exit\n");
    register int syscall asm("rsi") = SYS_EXIT;
    register int code asm("r8") = status;
    asm("syscall");

}
#endif

#ifndef MLIBC_BUILDING_RTDL
int sys_clock_get(int clock, time_t *secs, long *nanos) {
    sys_libc_log("[mlibc]   Calling sys_clock_get\n");
    return 0;
}
#endif

int sys_open(const char *path, int flags, int mode, int *fd) {

    sys_libc_log("[mlibc]   Calling sys_open\n");
    register int syscall asm ("rsi")= SYS_OPEN;

    register const char * r8 asm("r8") = path;
    register int r9 asm("r9") = flags;

    register int r15 asm ("r15");
    asm("syscall" 
            : "=r"(r15)
            : "r" (r9), "r"(r8)
            : "rcx", "r11", "memory");

    int res = r15;
    mlibc::infoLogger() << "[mlibc] Sys open got fd # " << res << frg::endlog;
    *fd = r15;

    return 0;
}

int sys_close(int fd) {
    int ret;
    int sys_errno;

    return 0;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {

    sys_libc_log("[mlibc]   Calling sys_tcb_set\n");

    register int syscall asm("rsi") = SYS_READ;
    register int r8 asm("r8") = fd;
    register char * r9 asm("r9") = (char*)buf;
    register size_t r10 asm("r10") = count;

    register size_t r15 asm("r15");
    asm("syscall"
            : "=r" (r15)
            : "r"(syscall), "r"(r8) , "r"(r9), "r"(r10)
            : "rcx", "r11");


    *bytes_read = r15;
    mlibc::infoLogger() << "[mlibc] SYS_READ read " << *bytes_read << " bytes" <<  frg::endlog;

    return 0;
}

#ifndef MLIBC_BUILDING_RTDL
int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {


    sys_libc_log("[mlibc]   Calling sys_write\n");

    register int syscall asm("rsi") = SYS_WRITE;
    register int _fd asm("r8") = fd;
    register char * _buf asm("r9") = (char*)buf;
    register size_t _count asm("r10") = count;

    register size_t bw asm("r15");
    *bytes_written = bw;

    return 0;
}
#endif


int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
    sys_libc_log("[mlibc]   Calling sys_seek\n");
    
    register int syscall asm("rsi") = SYS_SEEK;

    register int r8 asm("r8") = fd;
    register off_t r9 asm("r9") = offset;
    register int r10 asm("r10") = whence;
    register int r15 asm("r15");

    asm ("syscall"
            : "=r" (r15)
            : "r"(syscall), "r"(r8), "r"(r9) , "r"(r10)
            : "rcx", "r11");


    *new_offset = r15;
    mlibc::infoLogger() << "[mlibc] SYS_SEEK new offset " << *new_offset << " bytes" <<  frg::endlog;

    return 0;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags,
		int fd, off_t offset, void **window) {
    __ensure(flags & MAP_ANONYMOUS);

    sys_libc_log("[mlibc]   Calling sys_vm_map\n");

    register int rsi asm("rsi") = SYS_VM_MAP;
    register void* r8 asm("r8") = hint;
    register size_t r9 asm("r9") = size;
    register int r10 asm("r10") = prot;
    register int r12 asm("r12") = flags;
    register int r13 asm("r13") = fd;
    register off_t r14 asm("r14") = offset;

    register void *  r15 asm("r15");

    asm volatile("syscall" 
            : "=r"(r15) 
            :   "r" (rsi), "r" (r8),  
                "r" (r9), "r" (r10) , 
                "r" (r12) , "r" (r13), "r" (r14)
            : "rcx", "r11", "memory");

    *window = (void*)r15;
    //mlibc::infoLogger() << "[mlibc] Got vm_map return value " << *window << frg::endlog;

    return 0;
}

int sys_vm_unmap(void *pointer, size_t size) {
    // TODO
    sys_libc_log("[mlibc]   Calling sys_vm_unmap\n");
    return sys_anon_free(pointer, size);
}


int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {

    sys_libc_log("[mlibc]   Calling sys_vm_unmap\n");
    // TODO

	return 0;
}

int sys_futex_wake(int *pointer) {

    sys_libc_log("[mlibc]   Calling sys_futex_wake\n");


	return 0;
}

// All remaining functions are disabled in ldso.
#ifndef MLIBC_BUILDING_RTDL

int sys_clone(void *entry, void *user_arg, void *tcb, pid_t *tid_out) {
	void *sp = prepare_stack(entry, user_arg, tcb);
    int tid;

    asm volatile ("syscall"
        : "=a"(tid)
        : "a"(67), "D"(__mlibc_start_thread), "S"(sp), "d"(tcb)
        : "rcx", "r11");

	if (tid_out)
		*tid_out = tid;

	return 0;
}

void sys_thread_exit() {
	asm volatile ("syscall"
            :
            : "a"(68)
            : "rcx", "r11");
	__builtin_trap();
}

int sys_sleep(time_t *secs, long *nanos) {
    long ms = (*nanos / 1000000) + (*secs * 1000);
    asm volatile ("syscall"
        :
        : "a"(6), "D"(ms)
        : "rcx", "r11");
    *secs = 0;
    *nanos = 0;
    return 0;
}

int sys_fork(pid_t *child) {
    return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    int ret;
    int sys_errno;

    /* TODO */
    return 0;
}

pid_t sys_getpid() {
    pid_t pid;

    /* TODO */
    return pid;
}

pid_t sys_getppid() {
    pid_t ppid;

    /* TODO */
    return ppid;
}

#endif // MLIBC_BUILDING_RTDL

} // namespace mlibc
