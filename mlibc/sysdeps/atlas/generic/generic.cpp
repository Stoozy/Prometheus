#include <bits/ensure.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mlibc/all-sysdeps.hpp>
#include <mlibc/debug.hpp>
#include <mlibc/thread-entry.hpp>

#define SYS_EXIT 0
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_LOG_LIBC 5
#define SYS_VM_MAP 6
#define SYS_SEEK 7
#define SYS_TCB_SET 8

#define SYSCALL_NA0(call)                                                      \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall)                                                \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA1(call, arg0)                                                \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(r8)                                       \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA2(call, arg0, arg1)                                          \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    register auto r9 asm("r9") = (arg1);                                       \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(r8), "r"(r9)                              \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA3(call, arg0, arg1, arg2)                                    \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    register auto r9 asm("r9") = (arg1);                                       \
    register auto r10 asm("r10") = (arg2);                                     \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(r8), "r"(r9), "r"(r10)                    \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA4(call, arg0, arg1, arg2, arg3)                              \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    register auto r9 asm("r9") = (arg1);                                       \
    register auto r10 asm("r10") = (arg2);                                     \
    register auto r12 asm("r12") = (arg3);                                     \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(r8), "r"(r9), "r"(r10), "r"(r12)          \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA5(call, arg0, arg1, arg2, arg3, arg4)                        \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    register auto r9 asm("r9") = (arg1);                                       \
    register auto r10 asm("r10") = (arg2);                                     \
    register auto r12 asm("r12") = (arg3);                                     \
    register auto r13 asm("r13") = (arg4);                                     \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(r8), "r"(r9), "r"(r10), "r"(r12),         \
                   "r"(r13),                                                   \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA6(call, arg0, arg1, arg2, arg3, arg4, arg5)                  \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register auto r8 asm("r8") = (arg0);                                       \
    register auto r9 asm("r9") = (arg1);                                       \
    register auto r10 asm("r10") = (arg2);                                     \
    register auto r12 asm("r12") = (arg3);                                     \
    register auto r13 asm("r13") = (arg4);                                     \
    register auto r14 asm("r14") = (arg5);                                     \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(r8), "r"(r9), "r"(r10), "r"(r12),         \
                   "r"(r13), "r"(r14)                                          \
                 : "rcx", "r11", "memory");                                    \
  });

namespace mlibc {

void sys_libc_log(const char *message) {
  register int ret asm("r15");

  SYSCALL_NA1(SYS_LOG_LIBC, message);

  return;
}

void sys_libc_panic() {
  mlibc::infoLogger() << "\e[31mmlibc: panic!" << frg::endlog;
  for (;;)
    ;
}

int sys_tcb_set(void *pointer) {
  register int ret asm("r15");
  SYSCALL_NA1(SYS_TCB_SET, pointer);

  int r = ret;
  mlibc::infoLogger() << "Got tcb_set return " << r << frg::endlog;
  return ret;
}

int sys_anon_allocate(size_t size, void **pointer) {

  int errno = sys_vm_map(NULL, size, PROT_EXEC | PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS, -1, 0, pointer);

  return errno;
}

int sys_anon_free(void *pointer, size_t size) {
  // TODO
  return 0;
}

#ifndef MLIBC_BUILDING_RTDL
void sys_exit(int status) {
  register int ret asm("r15");
  SYSCALL_NA1(SYS_EXIT, status);
}
#endif

#ifndef MLIBC_BUILDING_RTDL
int sys_clock_get(int clock, time_t *secs, long *nanos) { return 0; }
#endif

int sys_open(const char *path, int flags, int *fd) {
  register int ret asm("r15");
  SYSCALL_NA2(SYS_OPEN, path, flags);

  *fd = ret;
  mlibc::infoLogger() << "[mlibc] Got sys_open return value " << *fd
                      << frg::endlog;

  return 0;
}

int sys_close(int fd) {
  int ret;
  int sys_errno;

  return 0;
}

int sys_read(int fd, void *buf, size_t count, ssize_t *bytes_read) {
  register size_t ret asm("r15");
  SYSCALL_NA3(SYS_READ, fd, buf, count);

  if (ret >= 0) {
    *bytes_read = ret;
  } else {
    return -1;
  }

  mlibc::infoLogger() << "[mlibc] SYS_READ read " << *bytes_read << " bytes"
                      << frg::endlog;

  return 0;
}

#ifndef MLIBC_BUILDING_RTDL
int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
  register size_t ret asm("r15");

  SYSCALL_NA3(SYS_WRITE, fd, buf, count);

  if (ret >= 0)
    *bytes_written = ret;
  else
    return -1;

  return 0;
}
#endif

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
  // TODO

  register off_t ret asm("r15");
  SYSCALL_NA3(SYS_SEEK, fd, offset, whence);

  *new_offset = ret;

  return 0;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd,
               off_t offset, void **window) {
  __ensure(flags & MAP_ANONYMOUS);

  register void *ret asm("r15");
  SYSCALL_NA6(SYS_VM_MAP, hint, size, prot, flags, fd, offset);

  *window = ret;
  mlibc::infoLogger() << "[mlibc] Got vm_map return value " << *window
                      << frg::endlog;

  return 0;
}

int sys_vm_unmap(void *pointer, size_t size) {
  // TODO
  return sys_anon_free(pointer, size);
}

int sys_futex_wait(int *pointer, int expected, const struct timespec *time) {
  // TODO
  // uint64_t err;
  // asm volatile ("syscall"
  //        : "=d"(err)
  //        : "a"(66), "D"(pointer), "S"(expected)
  //        : "rcx", "r11");

  // if (err) {
  //     return -1;
  // }

  return 0;
}

int sys_futex_wake(int *pointer) {
  // uint64_t err;
  // asm volatile ("syscall"
  //         : "=d"(err)
  //         : "a"(65), "D"(pointer)
  //         : "rcx", "r11");

  // if (err) {
  //     return -1;
  // }

  return 0;
}

// All remaining functions are disabled in ldso.
#ifndef MLIBC_BUILDING_RTDL

int sys_clone(void *entry, void *user_arg, void *tcb, pid_t *tid_out) {
  void *sp = prepare_stack(entry, user_arg, tcb);
  int tid;

  asm volatile("syscall"
               : "=a"(tid)
               : "a"(67), "D"(__mlibc_start_thread), "S"(sp), "d"(tcb)
               : "rcx", "r11");

  if (tid_out)
    *tid_out = tid;

  return 0;
}

void sys_thread_exit() {
  asm volatile("syscall" : : "a"(68) : "rcx", "r11");
  __builtin_trap();
}

int sys_sleep(time_t *secs, long *nanos) {
  long ms = (*nanos / 1000000) + (*secs * 1000);
  asm volatile("syscall" : : "a"(6), "D"(ms) : "rcx", "r11");
  *secs = 0;
  *nanos = 0;
  return 0;
}

int sys_fork(pid_t *child) { return 0; }

int sys_execve(const char *path, char *const argv[], char *const envp[]) {
  int ret;
  int sys_errno;

  asm volatile("syscall"
               : "=a"(ret), "=d"(sys_errno)
               : "a"(59), "D"(path), "S"(argv), "d"(envp)
               : "rcx", "r11");

  if (sys_errno != 0)
    return sys_errno;

  return 0;
}

pid_t sys_getpid() {
  pid_t pid;
  asm volatile("syscall" : "=a"(pid) : "a"(5) : "rcx", "r11", "rdx");
  return pid;
}

pid_t sys_getppid() {
  pid_t ppid;
  asm volatile("syscall" : "=a"(ppid) : "a"(14) : "rcx", "r11", "rdx");
  return ppid;
}

#endif // MLIBC_BUILDING_RTDL

} // namespace mlibc
