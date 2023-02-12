#include "cpu/idt.h"
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

extern volatile ProcessControlBlock *running;

extern PageTable *kernel_cr3;

extern __attribute__((noreturn)) void switch_to_process(Registers *new_stack,
                                                        PageTable *cr3);

extern void set_kernel_entry(void *rip);

static bool valid_fd(int fd) {
  if (fd < 0 || fd > MAX_PROC_FDS)
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
  for (;;)
    ;
  extern void kill_cur_proc(int ec);
  kill_cur_proc(status);
  // enable_irq();

  // ProcessControlBlock * next_proc = get_next_ready_process();
  // next_proc->state = RUNNING;
  // pqueue_remove(&ready_queue, running);
  // running = next_proc;

  // switch_to_process(&running->trapframe, running->cr3);
}

void sys_waitpid(pid_t pid, int *status, int flags, Registers *regs) {
  /* kprintf("sys_waitpid(): pid %d; flags %d; caller: %s (pid: %d);\n", pid, */
  /*         flags, running->name, running->pid); */

  if (pid < -1) {
    kprintf("Waiting on any child process with gid %d ... \n", abs(pid));
  } else if (pid == -1) {
    extern volatile ProcessControlBlock *running;

    if (running->children.count == 0) {
      for (;;)
        kprintf("NO children...\n ");
      regs->rdx = ECHILD;
      regs->rax = -1;
      return;
    }

    if (!running->childDied) {
      regs->rax = -1;
      regs->rdx = EINTR;
      return;
      ;
    }

    for (PQNode *pnode = running->children.first; pnode; pnode = pnode->next) {
      ProcessControlBlock *proc = pnode->pcb;
      if (proc->state == ZOMBIE) {
        // TODO: clean up the process (need a proper malloc)
        *status = proc->exit_code;
        regs->rax = proc->pid;
        kprintf("Got dead child %s (pid: %d; status %d)\n", proc->name,
                proc->pid, *status);
        pqueue_remove(&running->children, proc->pid);
        /* for(;;); */
        return;
      }
    }

    regs->rdx = ECHILD;
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

// void sys_open(const char *name, int flags, Registers *regs) {
// 
//   File *file;
// 
//   if (strcmp(name, ".") == 0) {
//     file = vfs_open(running->cwd, flags);
//     kprintf("Got file %s with size %d bytes\n", file->name, file->size);
//   } else
//     file = vfs_open(name, flags);
// 
//   if (!file) {
//     regs->rdx = ENOENT;
//     regs->rax = -1;
//     return;
//   }
// 
//   int fd = map_file_to_proc(running, file);
// 
//   // error on overflow
//   if (fd > MAX_PROC_FDS) {
//     regs->rdx = ENOMEM;
//     regs->rax = -1;
//     return;
//   }
// 
//   regs->rax = fd;
//   return;
// }
// 
// int sys_close(int fd) {
//   vmm_switch_page_directory(kernel_cr3);
//   unmap_fd_from_proc(running, fd);
//   kprintf("Closed fd %d\n", fd);
//   return 0;
// }

// void sys_read(int file, char *ptr, size_t len, Registers *regs) {

//   if (file > MAX_PROC_FDS) {
//     regs->rdx = EBADF;
//     regs->rax = -1;
//     return;
//   }

//   struct file *f = running->fd_table[file];
//   kprintf("File ptr %x. Name %s. Device %x\n", f, f->name, f->device);
//   int bytes_read = vfs_read(f, (u8 *)ptr, len);

//   regs->rax = bytes_read;
//   return;
// }

// int sys_write(int fd, char *ptr, int len) {
//   kprintf("sys_write(): FD is %d\n", fd);

//   if (!valid_fd(fd))
//     return -1;

//   File *file = running->fd_table[fd];
//   if (file) {
//     vfs_write(file, (uint8_t *)ptr, len);
//   } else {
//     kprintf("File not open!\n");
//     return -1;
//   }

// #ifdef SYSCALL_DEBUG
//   kprintf("sys_write(): %s; Buffer addr: 0x%x Length: %d\n", file->name, ptr,
//           len);
// #endif

//   return len;
// }

void *sys_vm_map(ProcessControlBlock *proc, void *addr, size_t size, int prot,
                 int flags, int fd, off_t offset) {

  kprintf("[MMAP] Requesting %llu bytes\n", size);
  kprintf("[MMAP] Hint : 0x%llx\n", addr);
  kprintf("[MMAP] Size : 0x%llx\n", size);
  kprintf("Current process at 0x%x\n", proc);

  // size isn't page aligned
  if (size % PAGE_SIZE != 0) {
    kprintf("[MMAP] Size wasn't page aligned");
    return NULL;
  }

  int pages = (size / PAGE_SIZE) + 1;
  void *phys_base = pmm_alloc_blocks(pages);

  if (phys_base == NULL) {
    // out of memory
    for (;;)
      kprintf("Out of memory\n");
    return NULL;
  }

  memset(PAGING_VIRTUAL_OFFSET + phys_base, 0, pages * PAGE_SIZE);

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

  // if (flags & MAP_SHARED) {
  //   if (!proc->fd_table[fd]) {
  //     for (;;)
  //       kprintf("Called mmap on invalid fd...");
  //   }

  //   if (strcmp(proc->fd_table[fd]->name, "/dev/fb0") == 0) {
  //     extern struct fb_file g_framebuffer;
  //     g_framebuffer.data = phys_base;
  //   }
  // }

  return virt_base;
}

// off_t sys_seek(int fd, off_t offset, int whence) {

//   File *file = running->fd_table[fd];

//   if (!file) {
//     kprintf("File dne: %d\n", fd);
//     return -1;
//   }

//   kprintf("[SYS_SEEK] Name %s\n", file->name);
//   kprintf("[SYS_SEEK] FD addr: %llx\n", file);
//   kprintf("[SYS_SEEK] FD is %d. Offset is %d. Whence is %d\n", fd, offset,
//           whence);

//   switch (whence) {
//   case SEEK_CUR:
//     kprintf("[SYS_SEEK] whence is SEEK_CUR\n");
//     running->fd_table[fd]->position += offset;
//     break;
//   case SEEK_SET:
//     kprintf("[SYS_SEEK] whence is SEEK_SET\n");
//     running->fd_table[fd]->position = offset;
//     break;
//   case SEEK_END:
//     kprintf("[SYS_SEEK] whence is SEEK_END\n");
//     running->fd_table[fd]->position = running->fd_table[fd]->size + offset;
//     break;
//   default:
//     kprintf("[SYS_SEEK] Whence is none\n");
//     break;
//   }

//   kprintf("\n");
//   return running->fd_table[fd]->position;
// }

// void sys_fstat(int fd, VfsNodeStat *vns, Registers *regs) {
//   if (fd > MAX_PROC_FDS || fd < 0)
//     kprintf("[SYS_STAT] Invalid FD");

//   File *file = running->fd_table[fd];

//   if (!file) {
//     regs->rdx = EBADF;
//     regs->rax = -1;
//     return;
//   }

//   kprintf("getting stat for %s\n", file->name);

//   vns->filesize = file->size;
//   vns->type = file->type;
//   vns->inode = file->inode;

//   regs->rax = 0;

//   return;
// }

// void sys_stat(const char *path, VfsNodeStat *statbuf, Registers *regs) {
//   int ret = vfs_stat(path, statbuf);
//   if (ret) {
//     regs->rdx = ENOENT;
//     regs->rax = -1;
//     return;
//   }

//   regs->rax = ret;
// }

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

// void sys_execve(char *name, char **argvp, char **envp) {
//   kprintf("sys_exec: %s\n", name);

//   extern volatile ProcessQueue ready_queue;
//   extern void dump_pqueue(ProcessQueue *);
//   kprintf("Before removing\n");
//   dump_pqueue(&ready_queue);
//   pqueue_remove(&ready_queue, running->pid);

//   char *name_cp = kmalloc(strlen(name) + 1);
//   strcpy(name_cp, name);

//   char *args_cp[count_args(argvp)];
//   char *env_cp[count_args(envp)];

//   kprintf("Called exec on %s\n", name);
//   kprintf("ARGS:  \n");
//   int n = 0;
//   while (argvp[n]) {
//     size_t strl = strlen(argvp[n]) + 1;
//     args_cp[n] = kmalloc(strl);
//     strcpy(args_cp[n], argvp[n]);

//     kprintf("%s\n", argvp[n]);
//     kprintf("%s\n", args_cp[n]);
//     n++;
//   }

//   args_cp[n] = NULL;

//   kprintf("ENV:  \n");
//   n = 0;
//   while (envp[n]) {
//     size_t strl = strlen(envp[n]) + 1;
//     env_cp[n] = kmalloc(strl);
//     strcpy(env_cp[n], envp[n]);
//     kprintf("%s\n", envp[n]);
//     kprintf("%s\n", env_cp[n]);
//     n++;
//   }

//   env_cp[n] = NULL;

//   extern void load_pagedir(PageTable *);
//   load_pagedir(kernel_cr3);

//   ProcessControlBlock *new = create_elf_process(name_cp, args_cp, env_cp);

//   if (running->parent) {
//     new->parent = running->parent;
//     // pqueue_remove(&running->parent->children, running->pid);
//     pqueue_push(&running->parent->children, new);
//     dump_pqueue(&running->parent->children);
//   }

//   pqueue_push(&ready_queue, new);

//   for (int i = 0; i < MAX_PROC_FDS; i++) {
//     // if (running->fd_table[i]->mode & FD_CLOEXEC)
//       // continue;

//     new->fd_table[i] = running->fd_table[i];
//   }

//   kprintf("[exec] scheduling \n");
//   // asm volatile("sti; int $41; cli");
//   schedule(&running->trapframe);

//   // running = new;

//   // kprintf("process (%s pid: %d) cr3 at %x\n", running->name, running->pid,
//   //         running->cr3);
//   // kprintf("Entrypoint 0x%x\n", running->trapframe.rip);

//   // get_cpu_struct(0)->syscall_kernel_stack = running->kstack +
//   // PAGING_VIRTUAL_OFFSET; switch_to_process(&running->trapframe,
//   // running->cr3);
//   //
// }

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

// int sys_ioctl(int fd, uint32_t req, void *arg) {
//   kprintf("Request is %x\n", req);
//   kprintf("FD is %d\n", fd);
//   if (!(valid_fd(fd))) {
//     return -1; // Invalid fd
//   }

//   File *file = running->fd_table[fd];
//   kprintf("File is at %x name: %s\n", file, file->name);

//   if (file->fs->ioctl)
//     return file->fs->ioctl(file, req, arg);

//   // kprintf("Name is %s\n", file->name);
//   return -1;
// }

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

// int sys_readdir(int handle, struct dirent *buffer, size_t max_size) {
//   kprintf("Dirent buffer is at %x\n", buffer);

//   if (max_size < sizeof(DirectoryEntry))
//     return -1;

//   File *file = running->fd_table[handle];
//   if (!file) {
//     kprintf("Invalid open stream\n");
//     return -1;
//   }

//   kprintf("Reading entries from %s\n", file->name);
//   DirectoryEntry *entry = vfs_readdir(file);
//   if (entry) {
//     *buffer = *entry;
//     return 0;
//   }

//   return -1;
// }

// int sys_fcntl(int fd, int request, va_list args) {

//   switch (request) {
//   case F_SETFD: {
//     kprintf("F_SETFD\n");
//     if (valid_fd(fd)) {
//       File *fp = running->fd_table[fd];
//       if (fp) {
//         int flags = va_arg(args, int);
//         fp->mode |= flags;
//         return 0;
//       }
//     }
//     return -1;
//   }
//   case F_GETFD: {
//     kprintf("F_GETFD\n");
//     if (valid_fd(fd)) {
//       File *fp = running->fd_table[fd];
//       if (fp) {
//         return fp->mode;
//       }
//     }
//     break;
//   }
//   case F_GETFL: {
//     if (valid_fd(fd)) {
//       File *fp = running->fd_table[fd];
//       if (fp) {
//         return fp->status;
//       }
//     }
//     return -1;
//     break;
//   }
//   case F_SETFL: {
//     kprintf("F_SETFL\n");
//     if (valid_fd(fd)) {
//       File *fp = running->fd_table[fd];
//       if (fp) {
//         int status = va_arg(args, int);
//         fp->status = status;
//         return 0;
//       }
//     }
//     return -1;
//     break;
//   }
//   default: {
//     kprintf("unknown request\n");
//     return -1;
//   }
//   }

//   return 0;
// }

// int sys_poll(struct pollfd *fds, uint32_t count, int timeout) {
//   int events = 0;
//   // kprintf("[POLL] pollfd ptr %x; count %u; Timeout %d;\n", fds, count,
//   // timeout);
//   //  forget timeout, just loop forever
//   for (uint32_t i = 0; i < count; i++) {
//     int fd = fds[i].fd;
//     if (valid_fd(fd)) {
//       struct file *file = running->fd_table[fd];
//       if (!file->fs->poll)
//         kprintf("Poll is not implemented for %s :(\n", file->name);
//       else {
//         events += file->fs->poll(file, &fds[i], timeout);
//       }
//     } else {
//       kprintf("[POLL]   Invalid fd %d\n", fd);
//     }
//   }

//   return events;
// }

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
    // sys_open((char *)regs->rdi, (int)regs->rsi, regs);
    break;
  }
  case SYS_CLOSE: {
    kprintf("[SYS]  CLOSE CALLED\n");
    // regs->rax = sys_close(regs->rdi);
    break;
  }
  case SYS_READ: {
    // kprintf("[SYS]  READ CALLED\n");
    // sys_read(regs->rdi, (char *)regs->rsi, regs->rdx, regs);
    break;
  }
  case SYS_WRITE: {
    // kprintf("[SYS]  WRITE CALLED\n");
    // regs->rax = sys_write(regs->rdi, (char *)regs->rsi, regs->rdx);
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
    int fd = regs->rdi;
    off_t off = regs->rsi;
    int whence = regs->rdx;
    // regs->rax = sys_seek(fd, off, whence);
    kprintf("Returning offset %d\n", regs->rax);

    break;
  }
  case SYS_TCB_SET: {
    kprintf("[SYS]  TCB_SET CALLED\n");
    regs->rax = sys_tcb_set((void *)regs->rdi);
    break;
  }
  case SYS_IOCTL: {
    kprintf("[SYS]  IOCTL CALLED\n");
    int fd = regs->rdi;
    unsigned long req = regs->rsi;
    void *arg = (void *)regs->rdx;

    // regs->rax = sys_ioctl(fd, req, arg);
    break;
  }
  case SYS_STAT: {
    kprintf("[SYS]  STAT CALLED\n");
    // sys_stat((const char *)regs->rdi, (VfsNodeStat *)regs->rsi, regs);
    break;
  }
  case SYS_FSTAT: {
    kprintf("[SYS]  FSTAT CALLED\n");
    // sys_fstat(regs->rdi, (VfsNodeStat *)regs->rsi, regs);
    break;
  }
  case SYS_GETPID: {
    regs->rax = sys_getpid();
    break;
  }

  case SYS_FCNTL: {
    // regs->rax = sys_fcntl(regs->rdi, regs->rsi, (void *)regs->rdx);
    break;
  }
  case SYS_POLL: {
    // regs->rax = sys_poll((struct pollfd *)regs->rdi, regs->rsi, regs->rdx);
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
    // regs->rax = sys_readdir(regs->rdi, (struct dirent*)regs->rsi, regs->rdx, regs);
    break;
  }
  case SYS_FORK: {
    sys_fork(regs);
    break;
  }
  case SYS_EXEC: {
    // sys_execve((char *)regs->rdi, (char **)regs->rsi, (char **)regs->rdx);
    break;
  }
  case SYS_WAIT: {
    sys_waitpid((pid_t)regs->rdi, (int *)regs->rsi, (int)regs->rdx, regs);
    break;
  }
  case SYS_DBG_PUTC: {
    /* TODO */
    kprintf("Called DBG_PUTC\n");
    break;
  }
  case SYS_DBG_GETC: {
    /* TODO */
    kprintf("Called DBG_GETC\n");
    break;
  }
  case SYS_DBG_EXCH: {
    /* TODO */
    kprintf("Called DBG_EXCH\n");
    break;
  }
  default: {
    kprintf("Invalid syscall %d\n", syscall);
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
