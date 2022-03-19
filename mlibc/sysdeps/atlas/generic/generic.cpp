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
const int SYS_ANON_ALLOC    = 7;


void sys_libc_log(const char *message) {
    register int syscall asm("rsi") = SYS_LOG_LIBC;
    register const char * msg  asm("r8") = message;
    asm("syscall");
}

void sys_libc_panic() {
    mlibc::infoLogger() << "\e[31mmlibc: panic!" << frg::endlog;
    asm volatile ("syscall" :
            : "a"(12), "D"(1)
            : "rcx", "r11", "rdx");
}

int sys_tcb_set(void *pointer) {
    // TODO
    return -1;
}

int sys_anon_allocate(size_t size, void **pointer) {
	//void *ret;
    //int sys_errno;

	//asm volatile ("syscall"
    //        : "=a"(ret), "=d"(sys_errno)
	//		: "a"(9), "D"(0), "S"(size)
	//		: "rcx", "r11");

    //if (!ret)
    //    return sys_errno;

	//*pointer = ret;
    
    register int syscall asm("rsi") = SYS_ANON_ALLOC;
    register size_t sz asm("r8") = size;

    asm("syscall");

    register void* ret asm("r15");
    *pointer = ret;

    return 0;
}

int sys_anon_free(void *pointer, size_t size) {
    //int unused_return;
    //int sys_errno;

	//asm volatile ("syscall"
    //        : "=a"(unused_return), "=d"(sys_errno)
	//		: "a"(11), "D"(pointer), "S"(size)
	//		: "rcx", "r11");

    //if (unused_return)
    //    return sys_errno;

    return 0;
}

#ifndef MLIBC_BUILDING_RTDL
void sys_exit(int status) {
    register int syscall asm("rsi") = SYS_EXIT;
    register int code asm("r8") = status;
    asm("syscall");

    //asm volatile ("syscall" :
    //        : "a"(12), "D"(status)
    //        : "rcx", "r11", "rdx");
    
}
#endif

#ifndef MLIBC_BUILDING_RTDL
int sys_clock_get(int clock, time_t *secs, long *nanos) {
    return 0;
}
#endif

int sys_open(const char *path, int flags, int *fd) {
    register int syscall asm ("rsi")= SYS_OPEN;
    register const char * fp asm("r8") = path;
    register int _flags asm("r9") = flags;
    register int * _fd asm("r10") = fd;

    asm("syscall");

    register int ret asm ("r15");
    *fd = ret;


    //int ret;
    //int sys_errno;

    //asm volatile ("syscall"
    //        : "=a"(ret), "=d"(sys_errno)
    //        : "a"(2), "D"(path), "S"(flags), "d"(0)
    //        : "rcx", "r11");

    //if (ret == -1)
    //    return sys_errno;

    //*fd = ret;
    return 0;
}

int sys_close(int fd) {
    int ret;
    int sys_errno;

    // TODO
    //asm volatile ("syscall"
    //        : "=a"(ret), "=d"(sys_errno)
    //        : "a"(3), "D"(fd)
    //        : "rcx", "r11");

    //if (ret == -1)
    //    return sys_errno;

    return 0;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {

    register int syscall asm("rsi") = SYS_READ;
    register int _fd asm("r8") = fd;
    register char * _buf asm("r9") = (char*)buf;
    register size_t _count asm("r10") = count;
    asm("syscall");

    register size_t br asm("r15");
    *bytes_read = br;

    return 0;
}

#ifndef MLIBC_BUILDING_RTDL
int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
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
    // TODO
    //off_t ret;
    //int sys_errno;

    //asm volatile ("syscall"
    //        : "=a"(ret), "=d"(sys_errno)
    //        : "a"(8), "D"(fd), "S"(offset), "d"(whence)
    //        : "rcx", "r11");

    //if (ret == -1)
    //    return sys_errno;

    //*new_offset = ret;
    return 0;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags,
		int fd, off_t offset, void **window) {
    __ensure(flags & MAP_ANONYMOUS);

    register int syscall asm("rsi") = SYS_VM_MAP;
    register void* addr asm("r8") = hint;
    register size_t sz asm("r9") = size;
    register int _prot asm("r10") = prot;
    register int _flags asm("r10") = flags;
    register int _fd asm("r12") = fd;
    register off_t off asm("r13") = offset;

    asm("syscall");

    register void * ret asm("r15");
    *window = ret;

    return 0;
    // TODO
    //void *ret;
    //int sys_errno;

    // mlibc::infoLogger() << "calling sys_vm_map with size: " << size << frg::endlog;

    //asm volatile ("syscall"
    //        : "=a"(ret), "=d"(sys_errno)
	//		: "a"(9), "D"(hint), "S"(size)
	//		: "rcx", "r11");

    //if (!ret)
    //    return sys_errno;

	//*window = ret;

    //return 0;
}

int sys_vm_unmap(void *pointer, size_t size) {
    // TODO
    return sys_anon_free(pointer, size);
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
    // TODO
    //uint64_t err;
    //asm volatile ("syscall"
    //        : "=d"(err)
    //        : "a"(66), "D"(pointer), "S"(expected)
    //        : "rcx", "r11");

    //if (err) {
    //    return -1;
    //}

	return 0;
}

int sys_futex_wake(int *pointer) {
    //uint64_t err;
    //asm volatile ("syscall"
    //        : "=d"(err)
    //        : "a"(65), "D"(pointer)
    //        : "rcx", "r11");

    //if (err) {
    //    return -1;
    //}

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
    pid_t ret;
    int sys_errno;

    asm volatile ("syscall"
            : "=a"(ret), "=d"(sys_errno)
            : "a"(57)
            : "rcx", "r11");

    if (ret == -1)
        return sys_errno;

    *child = ret;
    return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    int ret;
    int sys_errno;

    asm volatile ("syscall"
            : "=a"(ret), "=d"(sys_errno)
            : "a"(59), "D"(path), "S"(argv), "d"(envp)
            : "rcx", "r11");

    if (sys_errno != 0)
        return sys_errno;

    return 0;
}

pid_t sys_getpid() {
    pid_t pid;
    asm volatile ("syscall" : "=a"(pid)
            : "a"(5)
            : "rcx", "r11", "rdx");
    return pid;
}

pid_t sys_getppid() {
    pid_t ppid;
    asm volatile ("syscall" : "=a"(ppid)
            : "a"(14)
            : "rcx", "r11", "rdx");
    return ppid;
}

#endif // MLIBC_BUILDING_RTDL

} // namespace mlibc
