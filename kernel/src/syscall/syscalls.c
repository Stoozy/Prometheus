#include "cpu/idt.h"
#include "fs/tmpfs.h"
#include <abi-bits/seek-whence.h>
#include <abi-bits/vm-flags.h>
#include <config.h>
#include <cpu/cpu.h>
#include <fs/vfs.h>
#include <libk/kmalloc.h>
#include <libk/kprintf.h>
#include <libk/typedefs.h>
#include <libk/util.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <proc/proc.h>
#include <stdint.h>
#include <string/string.h>
#include <sys/queue.h>
#include <sys/wait.h>
#include <syscall/syscalls.h>

#include <abi-bits/fcntl.h>
#include <linux/poll.h>
#include <proc/elf.h>
#include <unistd.h>

#include <abi-bits/auxv.h>
#include <stdarg.h>

#include <abi-bits/errno.h>
#include <cpu/idt.h>

#include <drivers/fb.h>

typedef long int off_t;

extern ProcessControlBlock *running;

extern PageTable *kernel_cr3;

extern __attribute__((noreturn)) void switch_to_process(Registers *new_stack,
                                                        PageTable *cr3);

extern void set_kernel_entry(void *rip);

static bool valid_fd(int fd) {
  if (fd < 0 || fd > MAX_PROC_FDS)
    return false;

  if (!running->fd_table[fd])
    return false;

  return true;
}

char *__env = {0};
/* pointer to array of char * strings that define the current environment
 * variables */
char **environ = &__env;

void sys_log_libc(const char *message) {
  disable_irq();
  extern void turn_color_on();
  extern void turn_color_off();

  turn_color_on();
  kprintf("%s", message);
  turn_color_off();
  // enable_irq();
}

void sys_exit(int status) {
  kprintf("sys_exit(): status: %d; called by %s (pid: %d)\n", status,
          running->name, running->pid);
  extern void kill_cur_proc(int ec);
  kill_cur_proc(status);
}

void sys_waitpid(pid_t pid, int *status, int flags, Registers *regs) {
  // kprintf("sys_waitpid(): pid %d; flags %d; caller: %s (pid: %d);\n", pid,
  // flags, running->name, running->pid);

  // dump_pqueue(&running->children);
  if (pid < -1) {
    kprintf("Waiting on any child process with gid %d ... \n", abs(pid));
  } else if (pid == -1) {

    if (TAILQ_EMPTY(&running->children)) {
      // for (;;)
      //   kprintf("NO children...\n ");
      regs->rdx = ECHILD;
      regs->rax = -1;
      return;
    }

    // if (!running->childDied) {
    //   regs->rax = -1;
    //   regs->rdx = EINTR;
    //   return;
    // }

    ProcessControlBlock *proc;
    TAILQ_FOREACH(proc, &running->children, child_entries) {
      if (proc->state == ZOMBIE) {
        // TODO: clean up the process (need a proper malloc)

        *status = proc->exit_code;
        regs->rax = proc->pid;
        kprintf("Got dead child %s (pid: %d; status %d)\n", proc->name,
                proc->pid, *status);

        TAILQ_REMOVE(&running->children, proc, child_entries);

        return;
      }
    }

    // for (PQNode *pnode = running->children.first; pnode; pnode = pnode->next)
    // {
    //   ProcessControlBlock *proc = pnode->pcb;
    //   if (proc->state == ZOMBIE) {
    //     // TODO: clean up the process (need a proper malloc)

    //     *status = proc->exit_code;
    //     regs->rax = proc->pid;
    //     kprintf("Got dead child %s (pid: %d; status %d)\n", proc->name,
    //             proc->pid, *status);
    //     pqueue_remove(&running->children, proc->pid);

    //     // for (;;)
    //     //   ;
    //     return;
    //   }
    // }

    regs->rdx = EINTR;
    regs->rax = -1;
    return;

  } else if (pid == 0) {
    kprintf("Waiting on any child process with gid equal to the calling "
            "proc ... \n");
  } else if (pid > 0) {
    kprintf("Waiting for the child process with pid %d\n", pid);
    regs->rdx = ECHILD;
    regs->rax = -1;
  }

  regs->rdx = ENOSYS;
  regs->rax = -1;
  return;
}

int sys_open(const char *name, int flags, Registers *regs) {

  // if (strcmp(name, "/dev/fb0") == 0) {
  //   kprintf("opening /dev/fb0");
  //   for (;;)
  //     ;
  // }

  File *file = vfs_open(name, flags);

  static int count = 0;
  if (!file) {
    regs->rdx = ENOENT;
    return -1;
  }

  int fd = map_file_to_proc(running, file);

  // error on overflow
  if (fd > MAX_PROC_FDS) {
    regs->rdx = ENOMEM;
    return -1;
  }

  return fd;
}

// int sys_close(int fd) {
//   vmm_switch_page_directory(kernel_cr3);
//   unmap_fd_from_proc(running, fd);
//   kprintf("Closed fd %d\n", fd);
//   return 0;
// }

ssize_t sys_read(int fd, char *ptr, size_t len, Registers *regs) {

  if (!valid_fd(fd)) {
    regs->rdx = EBADF;
    return -1;
  }

  struct file *file = running->fd_table[fd];

  return vfs_read(file, (u8 *)ptr, len);
}

ssize_t sys_write(int fd, char *ptr, int len) {
  kprintf("sys_write(): FD is %d\n", fd);

  if (!valid_fd(fd)) {
    kprintf("Invalid fd");
    for (;;)
      ;
  }

  File *file = running->fd_table[fd];
  if (file) {
    return vfs_write(file, (uint8_t *)ptr, len);
  } else {
    kprintf("File not open!\n");
    return -1;
  }

  return 0;
}

void *sys_vm_map(ProcessControlBlock *proc, void *addr, size_t size, int prot,
                 int flags, int fd, off_t offset) {

  kprintf("[MMAP] Requesting %llu bytes\n", size);
  kprintf("[MMAP] Hint : 0x%llx\n", addr);
  kprintf("[MMAP] Size : 0x%llx\n", size);
  kprintf("Current process at 0x%x\n", proc);

  if (flags & MAP_SHARED)
    kprintf("MAP_SHARED");

  // size isn't page aligned
  if (size % PAGE_SIZE != 0) {
    kprintf("[MMAP] Size wasn't page aligned");
    return NULL;
  }

  int pages = DIV_ROUND_UP(size, PAGE_SIZE);
  void *phys_base;

  static int count = 0;
  if (flags & MAP_SHARED) {
    if (!valid_fd(fd)) {
      for (;;)
        kprintf("Called mmap on invalid fd...");
    }

    VFSNode *vnode = proc->fd_table[fd]->vn;
    TmpNode *tnode = vnode->private_data;

    // FIXME: only being used for /dev/fb0
    phys_base = tnode->dev.cdev.private_data - PAGING_VIRTUAL_OFFSET;
    kprintf("Framebuffer phys-base @ 0x%x\n", phys_base);
    kprintf("Called mmap on %s\n", tnode->name);
  } else {
    phys_base = pmm_alloc_blocks(pages);
    memset(PAGING_VIRTUAL_OFFSET + phys_base, 0, pages * PAGE_SIZE);
  }

  if (phys_base == NULL) {
    // out of memory
    for (;;)
      kprintf("Out of memory\n");
    return NULL;
  }

  void *virt_base = NULL;
  if (flags & MAP_FIXED && addr != NULL) {
    virt_base = addr;
  } else {
    virt_base = (void *)proc->mmap_base;
    proc->mmap_base += size;
  }

  kprintf("[MMAP] Found free chunk at 0x%x phys\n", phys_base);
  int page_flags = PAGE_USER | PAGE_PRESENT | PAGE_WRITE;

  if (flags & PROT_WRITE)
    page_flags |= PAGE_WRITE;

  kprintf("Virt base is %x\n", virt_base);

  vmm_map_range((void *)proc->cr3 + PAGING_VIRTUAL_OFFSET, virt_base, phys_base,
                size, page_flags);

  VASRangeNode *range = kmalloc(sizeof(VASRangeNode));
  range->virt_start = virt_base;
  range->phys_start = phys_base;
  range->size = pages * PAGE_SIZE;
  range->page_flags = page_flags;

  proc_add_vas_range(proc, range);

  kprintf("[MMAP] Returning 0x%x\n", virt_base);
  return virt_base;
}

off_t sys_seek(int fd, off_t offset, int whence) {
  if (!valid_fd(fd)) {
    kprintf("Invalid fd ...");
    return -1;
  }

  File *file = running->fd_table[fd];

  switch (whence) {
  case SEEK_CUR:
    kprintf("[SYS_SEEK] whence is SEEK_CUR\n");
    running->fd_table[fd]->pos += offset;
    break;
  case SEEK_SET:
    kprintf("[SYS_SEEK] whence is SEEK_SET\n");
    running->fd_table[fd]->pos = offset;
    break;
  case SEEK_END:
    kprintf("[SYS_SEEK] whence is SEEK_END\n");
    running->fd_table[fd]->pos = running->fd_table[fd]->vn->size + offset;
    break;
  default:
    kprintf("[SYS_SEEK] Whence is none\n");
    break;
  }

  kprintf("\n");
  return running->fd_table[fd]->pos;
}

void sys_fstat(int fd, VFSNodeStat *vns, Registers *regs) {
  if (fd > MAX_PROC_FDS || fd < 0)
    kprintf("[SYS_STAT] Invalid FD");

  File *file = running->fd_table[fd];

  if (!file) {
    regs->rdx = EBADF;
    regs->rax = -1;
    return;
  }

  vns->filesize = file->vn->size;
  vns->type = file->vn->type;
  vns->inode = (ino_t)file->vn->private_data;

  regs->rax = 0;

  return;
}

int sys_stat(const char *path, VFSNodeStat *statbuf, Registers *regs) {
  if (vfs_stat(path, statbuf)) {
    regs->rdx = ENOENT;
    return -1;
  }
  return 0;
}

int sys_tcb_set(void *ptr) {

  wrmsr(FSBASE, (uint64_t)ptr);
  running->fs_base = (uintptr_t)ptr;
  return 0;
}

int count_args(char **args) {
  int n = 0;
  while (args[n])
    n++;
  return n + 1;
}

void sys_execve(char *name, char **argvp, char **envp) {
  kprintf("sys_exec: %s\n", name);

  kprintf("Before removing\n");

  dump_readyq();

  TAILQ_REMOVE(&readyq, running, entries);

  // pqueue_remove(&ready_queue, running->pid);

  char *name_cp = kmalloc(strlen(name) + 1);
  strcpy(name_cp, name);

  char *args_cp[count_args(argvp)];
  char *env_cp[count_args(envp)];

  kprintf("Called exec on %s\n", name);
  kprintf("ARGS:  \n");
  int n = 0;
  while (argvp[n]) {
    size_t strl = strlen(argvp[n]) + 1;
    args_cp[n] = kmalloc(strl);
    strcpy(args_cp[n], argvp[n]);

    kprintf("%s\n", argvp[n]);
    kprintf("%s\n", args_cp[n]);
    n++;
  }

  args_cp[n] = NULL;

  kprintf("ENV:  \n");
  n = 0;
  while (envp[n]) {
    size_t strl = strlen(envp[n]) + 1;
    env_cp[n] = kmalloc(strl);
    strcpy(env_cp[n], envp[n]);
    kprintf("%s\n", envp[n]);
    kprintf("%s\n", env_cp[n]);
    n++;
  }

  env_cp[n] = NULL;

  extern void load_pagedir(PageTable *);
  load_pagedir(kernel_cr3);

  ProcessControlBlock *new = create_elf_process(name_cp, args_cp, env_cp);

  if (running->parent) {
    new->parent = running->parent;
    TAILQ_REMOVE(&running->parent->children, running, child_entries);
    // pqueue_remove(&running->parent->children, running->pid);
    // pqueue_push(&running->parent->children, new);
    //  dump_pqueue(&running->parent->children);

    TAILQ_INSERT_TAIL(&running->parent->children, new, child_entries);
  }

  TAILQ_INSERT_TAIL(&readyq, new, entries);

  // pqueue_push(&ready_queue, new);

  for (int i = 0; i < MAX_PROC_FDS; i++) {
    // TODO: check CLOEXEC
    new->fd_table[i] = running->fd_table[i];
  }

  new->cwd = strdup(running->cwd);

  kprintf("[exec] scheduling \n");
  asm volatile("sti; int $41; cli");
  // schedule(&running->trapframe);

  // running = new;

  // kprintf("process (%s pid: %d) cr3 at %x\n", running->name,
  // running->pid,
  //         running->cr3);
  // kprintf("Entrypoint 0x%x\n", running->trapframe.rip);

  // get_cpu_struct(0)->syscall_kernel_stack = running->kstack +
  // PAGING_VIRTUAL_OFFSET; switch_to_process(&running->trapframe,
  // running->cr3);
  //
}

void sys_fork(Registers *regs) {
  disable_irq();

  kprintf("sys_fork(): caller %s\n", running->name);
  dump_regs(regs);

  ProcessControlBlock *child_proc = clone_process(running, regs);
  register_process(child_proc);

  dump_regs(&child_proc->trapframe);
  regs->rax = child_proc->pid;

  return;
}

int sys_ioctl(int fd, uint32_t req, void *arg) {
  kprintf("Request is %x\n", req);
  kprintf("FD is %d\n", fd);
  if (!(valid_fd(fd))) {
    return -1; // Invalid fd
  }

  File *file = running->fd_table[fd];

  if (file->vn->ops->ioctl)
    return file->vn->ops->ioctl(file->vn, req, arg, 0);

  return -1;
}

pid_t sys_getpid() { return running->pid; }

int sys_dup(int fd, int flags) {

  if (!valid_fd(fd)) {
    kprintf("Invalid fd %d\n", fd);
    return -1;
  }

  File *file = running->fd_table[fd];
  int new_fd = map_file_to_proc(running, file);

  return new_fd;
}

int sys_dup2(int fd, int flags, int new_fd) {

  if (fd < 0 || fd > MAX_PROC_FDS) {
    kprintf("[DUP2] Invalid fd %d\n", fd);
    return -1;
  }

  File *file = running->fd_table[fd];

  if (!file) {
    kprintf("[DUP2] File doesn't exist\n");
    return -1;
  }

  // FIXME: call underlying fs close
  running->fd_table[new_fd] = NULL;

  // refer to the same file
  running->fd_table[new_fd] = file;
  // kprintf("New fd %d name: %s\n", new_fd, running->fd_table[new_fd]->name);

  return new_fd;
}

int sys_readdir(int handle, DirectoryEntry *buffer, size_t max_size) {
  kprintf("Dirent buffer is at %x\n", buffer);

  if (max_size < sizeof(DirectoryEntry))
    return -1;

  if (!valid_fd(handle)) {
    kprintf("Invalid handle for readdir\n");
    return -1;
  }

  File *file = running->fd_table[handle];

  if (vfs_readdir(file, buffer))
    return -1;

  return 0;
}

int sys_poll(struct pollfd *fds, uint32_t count, int timeout) {
  int events = 0;
  // kprintf("[POLL] pollfd ptr %x; count %u; Timeout %d;\n", fds, count,
  // timeout);

  for (uint32_t i = 0; i < count; i++) {
    int fd = fds[i].fd;
    if (valid_fd(fd)) {
      struct file *file = running->fd_table[fd];
      if (!file->vn->ops->poll) {

        TmpNode *tnode = file->vn->private_data;
        kprintf("Poll is not implemented for %s \n", tnode->name);
      } else {
        int revents = file->vn->ops->poll(file->vn, fds[i].events);

        if (revents) {
          fds[i].revents = revents;
          events++;
        }
      }

    } else {
      kprintf("[POLL]   Invalid fd %d\n", fd);
    }
  }

  return events;
}

int sys_chdir(const char *path) {
  char *name = kmalloc(strlen(path));
  running->cwd = strdup(path);
  kprintf("Changing directory to %s\n", name);
  return 0;
}

int sys_getcwd(char *buffer, size_t size) {
  if (size < strlen(running->cwd)) {
    kprintf("[get_cwd] name too small\n");
    for (;;)
      ;
  }

  strcpy(buffer, running->cwd);
  return 0;
}

int sys_mkdir(const char *path, mode_t mode) { return vfs_mkdir(path, mode); }

// stub
int sys_access(const char *filename, mode_t mode) {
  kprintf("called access on %s\n", filename);
  return F_OK;
}

void syscall_dispatcher(Registers *regs) {

  u64 syscall = regs->rax;

  running->trapframe = *regs;

  switch (syscall) {
  case SYS_EXIT: {
    sys_exit((int)regs->rdi);
    break;
  }
  case SYS_OPEN: {
    kprintf("[SYS]  OPEN CALLED by %s (%d)\n", running->name, running->pid);
    regs->rax = sys_open((char *)regs->rdi, (int)regs->rsi, regs);
    break;
  }
  case SYS_CLOSE: {
    kprintf("[SYS]  CLOSE CALLED\n");
    for (;;)
      ;
    break;
  }
  case SYS_READ: {
    regs->rax = sys_read(regs->rdi, (char *)regs->rsi, regs->rdx, regs);
    break;
  }
  case SYS_WRITE: {
    regs->rax = sys_write(regs->rdi, (char *)regs->rsi, regs->rdx);
    break;
  }
  case SYS_LOG_LIBC: {
    sys_log_libc((const char *)regs->rdi);
    break;
  }
  case SYS_VM_MAP: {
    kprintf("[SYS]  VM_MAP CALLED\n");
    void *ret = sys_vm_map(running, (void *)regs->rdi, regs->rsi, regs->rdx,
                           regs->r10, regs->r9, regs->r8);
    regs->rax = (u64)ret;

    break;
  }
  case SYS_SEEK: {
    kprintf("[SYS]  SEEK CALLED\n");
    regs->rax = sys_seek(regs->rdi, regs->rsi, regs->rdx);
    break;
  }
  case SYS_TCB_SET: {
    kprintf("[SYS]  TCB_SET CALLED\n");
    regs->rax = sys_tcb_set((void *)regs->rdi);
    break;
  }
  case SYS_IOCTL: {
    kprintf("[SYS]  IOCTL CALLED\n");
    regs->rax = sys_ioctl(regs->rdi, regs->rsi, (void *)regs->rdx);
    break;
  }
  case SYS_STAT: {
    kprintf("[SYS]  STAT CALLED\n");
    regs->rax =
        sys_stat((const char *)regs->rdi, (VFSNodeStat *)regs->rsi, regs);
    break;
  }
  case SYS_FSTAT: {
    kprintf("[SYS]  FSTAT CALLED\n");
    sys_fstat(regs->rdi, (VFSNodeStat *)regs->rsi, regs);
    break;
  }
  case SYS_GETPID: {
    regs->rax = sys_getpid();
    break;
  }
  case SYS_POLL: {
    regs->rax = sys_poll((struct pollfd *)regs->rdi, regs->rsi, regs->rdx);
    break;
  }
  case SYS_DUP: {
    regs->rax = sys_dup(regs->rdi, regs->rsi);
    break;
  }
  case SYS_DUP2: {
    regs->rax = sys_dup2(regs->rdi, regs->rsi, regs->rdx);
    break;
  }
  case SYS_READDIR: {
    kprintf("SYS_READDIR called\n");
    regs->rax = sys_readdir(regs->rdi, (DirectoryEntry *)regs->rsi, regs->rdx);
    break;
  }
  case SYS_FORK: {
    sys_fork(regs);
    break;
  }
  case SYS_EXEC: {
    sys_execve((char *)regs->rdi, (char **)regs->rsi, (char **)regs->rdx);
    break;
  }
  case SYS_WAIT: {
    sys_waitpid((pid_t)regs->rdi, (int *)regs->rsi, (int)regs->rdx, regs);
    break;
  }
  case SYS_CHDIR: {
    regs->rax = sys_chdir((const char *)regs->rdi);
    break;
  }
  case SYS_GETCWD: {
    kprintf("Get cwd called\n");
    // for (;;)
    //;
    regs->rax = sys_getcwd((char *)regs->rdi, regs->rsi);
    break;
  }
  case SYS_MKDIR: {
    regs->rax = sys_mkdir((const char *)regs->rdi, regs->rsi);
    break;
  }
  case SYS_ACCESS: {
    regs->rax = sys_access((const char *)regs->rdi, regs->rsi);
    break;
  }
  case SYS_CLOCK: {
    extern uint64_t g_ticks;
    kprintf("[SYS_CLOCK] called\n");
    regs->rax = g_ticks;
    break;
  }
  case SYS_SPAWN_THREAD: {
    break;
  }
  default: {
    kprintf("Invalid syscall %d\n", syscall);
    for (;;)
      ;
    break;
  }
  }
}

void sys_init() {
  cpu_init(0);
  LocalCpuData *lcd = get_cpu_struct(0);

  wrmsr(EFER, rdmsr(EFER) | 1); // enable syscall

  extern void enable_sce(); // syscall_entry.asm
  enable_sce();

  wrmsr(GSBASE, (u64)lcd);  //  GSBase
  wrmsr(KGSBASE, (u64)lcd); // KernelGSBase
  wrmsr(SFMASK, (u64)0);    // KernelGSBase

  extern void syscall_entry(); // syscall_entry.asm
  wrmsr(LSTAR, (u64)&syscall_entry);
}
