#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#define SYS_EXIT 0
#define SYS_OPEN 1
#define SYS_CLOSE 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_LOG_LIBC 5
#define SYS_VM_MAP 6
#define SYS_SEEK 7
#define SYS_TCB_SET 8

#define SYSCALL_NA0(call, arg0)                                                \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall)                                                \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA1(call, arg0)                                                \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register typeof(arg0) a0 asm("r8") = (arg0);                               \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(a0)                                       \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA2(call, arg0, arg1)                                          \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register typeof(arg0) a0 asm("r8") = (arg0);                               \
    register typeof(arg1) a1 asm("r9") = (arg1);                               \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(a0), "r"(a1)                              \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA3(call, arg0, arg1, arg2)                                    \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register typeof(arg0) a0 asm("r8") = (arg0);                               \
    register typeof(arg1) a1 asm("r9") = (arg1);                               \
    register typeof(arg2) a2 asm("r10") = (arg2);                              \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(a0), "r"(a1), "r"(a2)                     \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA4(call, arg0, arg1, arg2, arg3)                              \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register typeof(arg0) a0 asm("r8") = (arg0);                               \
    register typeof(arg1) a1 asm("r9") = (arg1);                               \
    register typeof(arg2) a2 asm("r10") = (arg2);                              \
    register typeof(arg3) a3 asm("r12") = (arg3);                              \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(a0), "r"(a1), "r"(a2), "r"(a3)            \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA5(call, arg0, arg1, arg2, arg3, arg4)                        \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register typeof(arg0) a0 asm("r8") = (arg0);                               \
    register typeof(arg1) a1 asm("r9") = (arg1);                               \
    register typeof(arg2) a2 asm("r10") = (arg2);                              \
    register typeof(arg3) a3 asm("r12") = (arg3);                              \
    register typeof(arg4) a4 asm("r13") = (arg4);                              \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4),  \
                 : "rcx", "r11", "memory");                                    \
  });

#define SYSCALL_NA6(call, arg0, arg1, arg2, arg3, arg4, arg5)                  \
  ({                                                                           \
    register uint64_t syscall asm("rsi") = call;                               \
    register typeof(arg0) a0 asm("r8") = (arg0);                               \
    register typeof(arg1) a1 asm("r9") = (arg1);                               \
    register typeof(arg2) a2 asm("r10") = (arg2);                              \
    register typeof(arg3) a3 asm("r12") = (arg3);                              \
    register typeof(arg4) a4 asm("r13") = (arg4);                              \
    register typeof(arg5) a5 asm("r14") = (arg5);                              \
    asm volatile("syscall"                                                     \
                 : "=r"(ret)                                                   \
                 : "r"(syscall), "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4),  \
                   "r"(a5)                                                     \
                 : "rcx", "r11", "memory");                                    \
  });

void sys_exit(int status) {
  int ret;
  SYSCALL_NA1(SYS_EXIT, status);
}

int sys_clock_get(int clock, time_t *secs, long *nanos) { return 0; }

int sys_open(const char *path, int flags, int *fd) {
  register int ret asm("r15");
  SYSCALL_NA2(SYS_OPEN, path, flags);

  *fd = ret;

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

  return 0;
}

int sys_write(int fd, const void *buf, size_t count, ssize_t *bytes_written) {
  register size_t ret asm("r15");

  SYSCALL_NA3(SYS_WRITE, fd, buf, count);

  if (ret >= 0)
    *bytes_written = ret;
  else
    return -1;

  return 0;
}

int sys_seek(int fd, off_t offset, int whence, off_t *new_offset) {
  // TODO

  register off_t ret asm("r15");
  SYSCALL_NA3(SYS_SEEK, fd, offset, whence);

  *new_offset = ret;

  return 0;
}

int sys_vm_map(void *hint, size_t size, int prot, int flags, int fd,
               off_t offset, void **window) {

  register void *ret asm("r15");
  SYSCALL_NA6(SYS_VM_MAP, hint, size, prot, flags, fd, offset);

  *window = ret;

  return 0;
}

int sys_vm_unmap(void *pointer, size_t size) {
  // TODO
  return 0;
}

int sys_sleep(time_t *secs, long *nanos) {
  long ms = (*nanos / 1000000) + (*secs * 1000);
  // asm volatile("syscall" : : "a"(6), "D"(ms) : "rcx", "r11");
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
