#include <abi-bits/seek-whence.h>
#include <abi-bits/vm-flags.h>
#include <config.h>
#include <cpu/cpu.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <proc/proc.h>
#include <stdint.h>
#include <string/string.h>
#include <syscall/syscalls.h>
#include <typedefs.h>
#include <unistd.h>
#include <util.h>

#include <abi-bits/fcntl.h>
#include <linux/poll.h>
#include <proc/elf.h>

#include <libc/abi-bits/auxv.h>
#include <stdarg.h>

typedef long int off_t;

extern PageTable *kernel_cr3;

static inline uint64_t rdmsr(uint64_t msr) {
  uint32_t low, high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint64_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

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

void sys_log_libc(const char *message) { kprintf(message); }

int sys_exit() {
  kill_current_proc();
  return 0;
}

int sys_open(const char *name, int flags, ...) {

  File *file = vfs_open(name, flags);
  // kprintf("Got file %s with size %d bytes\n", file->name, file->size);

  if (file == NULL)
    return -1;

  extern ProcessControlBlock *running;

  // error on overflow
  if (running->fd_length > MAX_PROC_FDS)
    return -1;

  int fd = map_file_to_proc(running, file);

  return fd;
}

int sys_close(int fd) {
  vmm_switch_page_directory(kernel_cr3);
  extern ProcessControlBlock *running;
  unmap_fd_from_proc(running, fd);
  kprintf("Closed fd %d\n", fd);
  return 0;
}

int sys_read(int file, char *ptr, size_t len) {

  extern ProcessControlBlock *running;
  struct file *f = running->fd_table[file];
  vfs_dump();
  // kprintf("File ptr %x. Name %s. Device %x\n", f, f->name, f->device);
  int bytes_read = vfs_read(f, (u8 *)ptr, len);

  return bytes_read;
}

int sys_write(int fd, char *ptr, int len) {
  extern ProcessControlBlock *running;

  // if (fd == 1 || fd == 2)
  // kprintf("Writing data to stdout or stderr: %s\n", ptr);

  if (!valid_fd(fd))
    return -1;

  File *file = running->fd_table[fd];
  if (file) {
    kprintf("Writing to %s\n", file->name);
    vfs_write(file, (uint8_t *)ptr, len);
  } else {
    kprintf("File not open!\n");
    for (;;)
      ;
  }

#ifdef SYSCALL_DEBUG
  kprintf("Called SYS_WRITE File: %d Buffer addr: 0x%x Length: %d\n", file, ptr,
          len);
#endif
  return len;
}

void *sys_vm_map(ProcessControlBlock *proc, void *addr, size_t size, int prot,
                 int flags, int fd, off_t offset) {

  kprintf("[MMAP] Requesting %llu bytes\n", size);
  kprintf("[MMAP] Hint : 0x%llx\n", addr);
  kprintf("[MMAP] Size : 0x%llx\n", size);
  kprintf("Current process at 0x%x\n", proc);

  if (valid_fd(fd)) {
    File *file = proc->fd_table[fd];
    int pages = (size / PAGE_SIZE);

    void *phys_base =
        (void *)((file->inode & ~(0xfff)) - PAGING_VIRTUAL_OFFSET);

    void *virt_base = NULL;
    if (flags & MAP_FIXED && addr != NULL) {
      virt_base = addr;
    } else {
      virt_base = (void *)proc->mmap_base;
      proc->mmap_base += pages * PAGE_SIZE;
    }

    int page_flags = PAGE_USER | PAGE_PRESENT | PAGE_WRITE;

    if (flags & PROT_WRITE)
      page_flags |= PAGE_WRITE;

    kprintf("Virt base is %x\n", virt_base);

    vmm_map_range((void *)((u64)proc->cr3 + PAGING_VIRTUAL_OFFSET), virt_base,
                  phys_base, size, page_flags);

    VASRangeNode *range = kmalloc(sizeof(VASRangeNode));
    range->virt_start = virt_base;
    range->phys_start = phys_base;
    range->size = pages * PAGE_SIZE;
    range->page_flags = page_flags;

    proc_add_vas_range(proc, range);

    return virt_base;
  }

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

  vmm_map_range((void *)((u64)proc->cr3 + PAGING_VIRTUAL_OFFSET), virt_base,
                phys_base, size, page_flags);

  VASRangeNode *range = kmalloc(sizeof(VASRangeNode));
  range->virt_start = virt_base;
  range->phys_start = phys_base;
  range->size = pages * PAGE_SIZE;
  range->page_flags = page_flags;

  proc_add_vas_range(proc, range);

  kprintf("[MMAP] Returning 0x%x\n", virt_base);

  return virt_base;

  kprintf("Returning NULL");
  return NULL;
}

off_t sys_seek(int fd, off_t offset, int whence) {

  extern ProcessControlBlock *running;
  File *file = running->fd_table[fd];

  if (!file) {
    kprintf("File dne: %d\n", fd);
    return -1;
  }

  kprintf("[SYS_SEEK] Name %s\n", file->name);
  kprintf("[SYS_SEEK] FD addr: %llx\n", file);
  kprintf("[SYS_SEEK] FD is %d. Offset is %d. Whence is %d\n", fd, offset,
          whence);

  switch (whence) {
  case SEEK_CUR:
    kprintf("[SYS_SEEK] whence is SEEK_CUR\n");
    running->fd_table[fd]->position += offset;
    break;
  case SEEK_SET:
    kprintf("[SYS_SEEK] whence is SEEK_SET\n");
    running->fd_table[fd]->position = offset;
    break;
  case SEEK_END:
    kprintf("[SYS_SEEK] whence is SEEK_END\n");
    running->fd_table[fd]->position = running->fd_table[fd]->size + offset;
    break;
  default:
    kprintf("[SYS_SEEK] Whence is none\n");
    break;
  }

  kprintf("\n");
  return running->fd_table[fd]->position;
}

int sys_fstat(int fd, VfsNodeStat *statbuf) {
  if (fd > MAX_PROC_FDS || fd < 0)
    kprintf("[SYS_STAT] Invalid FD");

  extern ProcessControlBlock *running;
  File *file = running->fd_table[fd];

  if (!file)
    kprintf("[SYS_FSTAT]  File not open\n");

  statbuf->filesize = file->size;
  statbuf->type = file->type;
  statbuf->inode = file->inode;

  return 0;
}

int sys_stat(const char *path, VfsNodeStat *statbuf) {
  if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0 ||
      strcmp(path, "/") == 0) {
    statbuf->inode = 0;
    statbuf->filesize = 0;
    statbuf->type = VFS_DIRECTORY;
    return 0;
  }

  vfs_get_stat(path, statbuf);
  return 0;
}

int sys_tcb_set(void *ptr) {

  wrmsr(FSBASE, (uint64_t)ptr);

  return 0;
}

// for (;;) {
//   kprintf("Called exec on %s\n", name);
//   kprintf("ARGS:  \n");
//   int n = 0;
//   while (argv[n]) {
//     kprintf("%s\n", argv[n]);
//     n++;
//   }
//   kprintf("ENV:  \n");
//   n = 0;
//   while (env[n]) {
//     kprintf("%s\n", env[n]);
//     n++;
//   }
// }

int sys_execve(char *name, char **argvp, char **envp) {
  extern ProcessControlBlock *running;

  // free the entire vas
  VASRangeNode *vas = running->vas;
  // while (vas->next) {
  //   size_t blocks = vas->size / PAGE_SIZE;
  //   kprintf("Freeing virt: %x; phys %x; blocks: %d\n", vas->virt_start,
  //           vas->phys_start, blocks);
  //   pmm_free_blocks((uintptr_t)vas->phys_start, blocks);

  //  VASRangeNode *tmp = vas;
  //  vas = vas->next;
  //  kfree(tmp);
  //}

  File *elf_file = vfs_open(name, O_RDONLY);
  u8 *elf_data = kmalloc(elf_file->size);
  vfs_read(elf_file, elf_data, elf_file->size);

  running->cr3 = pmm_alloc_block() + PAGING_VIRTUAL_OFFSET;
  //(void *)running->cr3 + PAGING_VIRTUAL_OFFSET;

  extern void load_pagedir(PageTable *);
  PageTable *tmp = vmm_get_current_cr3();
  load_pagedir(kernel_cr3);
  Auxval aux = load_elf_segments(running, elf_data);

  for (int i = 256; i < 512; i++)
    running->cr3->entries[i] = kernel_cr3->entries[i];

  load_pagedir(tmp);
  void *stack_ptr =
      pmm_alloc_blocks(STACK_BLOCKS) + STACK_SIZE + PAGING_VIRTUAL_OFFSET;
  void *stack_base = stack_ptr - (STACK_SIZE);

  vmm_map_range(running->cr3, stack_base - PAGING_VIRTUAL_OFFSET,
                stack_base - PAGING_VIRTUAL_OFFSET, STACK_SIZE,
                PAGE_USER | PAGE_WRITE | PAGE_PRESENT);

  VASRangeNode *range = kmalloc(sizeof(VASRangeNode));

  range->virt_start = stack_base;
  range->phys_start = stack_base;
  range->size = STACK_SIZE;
  range->page_flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
  range->next = NULL;

  proc_add_vas_range(running, range);

  uint64_t *stack = (uint64_t *)(stack_ptr);

  int envp_len;
  for (envp_len = 0; envp[envp_len] != NULL; envp_len++) {
    size_t length = strlen(envp[envp_len]);
    stack = (void *)stack - length - 1;
    memcpy(stack, envp[envp_len], length);
  }

  int argv_len;
  for (argv_len = 0; argvp[argv_len] != NULL; argv_len++) {
    size_t length = strlen(argvp[argv_len]);
    stack = (void *)stack - length - 1;
    memcpy(stack, argvp[argv_len], length);
  }

  stack = (uint64_t *)(((uintptr_t)stack / 16) * 16);

  if (((argv_len + envp_len + 1) & 1) != 0) {
    stack--;
  }

  *--stack = 0;
  *--stack = 0;
  *--stack = aux.entry;
  *--stack = AT_ENTRY;
  *--stack = aux.phent;
  *--stack = AT_PHENT;
  *--stack = aux.phnum;
  *--stack = AT_PHNUM;
  *--stack = aux.phdr;
  *--stack = AT_PHDR;

  uintptr_t old_rsp = (uintptr_t)stack_ptr;

  *--stack = 0; // end envp
  stack -= envp_len;
  for (int i = 0; i < envp_len; i++) {
    old_rsp -= strlen(envp[i]) + 1;
    stack[i] = old_rsp - PAGING_VIRTUAL_OFFSET;
  }

  *--stack = 0; // end argvp
  stack -= argv_len;
  for (int i = 0; i < argv_len; i++) {
    old_rsp -= strlen(argvp[i]) + 1;
    stack[i] = old_rsp - PAGING_VIRTUAL_OFFSET;
  }

  *--stack = argv_len; // argc

  stack_ptr -= (stack_ptr - (void *)stack);

  memset(&running->trapframe, 0, sizeof(Registers));
  running->trapframe.ss = 0x23;
  running->trapframe.rip = aux.ld_entry;
  running->trapframe.rsp = (uint64_t)stack_ptr - PAGING_VIRTUAL_OFFSET;

  running->trapframe.rflags = (uint64_t)0x202;
  running->trapframe.cs = (uint64_t)0x2b;

  running->cr3 = (void *)running->cr3 - PAGING_VIRTUAL_OFFSET;
  running->mmap_base = MMAP_BASE;
  ++running->pid;

  extern void switch_to_process(void *new_stack, PageTable *cr3);
  switch_to_process(&running->trapframe, running->cr3);

  // should never reach
  return 0;
}

int sys_fork(Registers *regs) {
  extern ProcessControlBlock *running;

  dump_regs(&running->trapframe);
  ProcessControlBlock *child_proc = clone_process(running, regs);
  register_process(child_proc);

  return child_proc->pid;
}

int sys_ioctl(int fd, unsigned long req, void *arg) {
  extern ProcessControlBlock *running;
  kprintf("Request is %x\n", req);
  kprintf("FD is %d\n", fd);
  if (!(valid_fd(fd))) {
    return -1; // Invalid fd
  }

  File *file = running->fd_table[fd];

  if (file->fs->ioctl)
    return file->fs->ioctl(file, req, arg);

  // kprintf("Name is %s\n", file->name);
  return -1;
}

pid_t sys_getpid() {
  extern ProcessControlBlock *running;
  return running->pid;
}

int sys_dup(int fd, int flags) {

  extern ProcessControlBlock *running;

  if (!valid_fd(fd)) {
    kprintf("Invalid fd %d\n", fd);
    return -1;
  }

  File *file = running->fd_table[fd];
  int new_fd = map_file_to_proc(running, file);

  return new_fd;
}

int sys_dup2(int fd, int flags, int new_fd) {

  extern ProcessControlBlock *running;

  if (fd < 0 || fd > MAX_PROC_FDS) {
    kprintf("[DUP2] Invalid fd %d\n", fd);
    return -1;
  }

  File *file = running->fd_table[fd];

  if (!file) {
    kprintf("[DUP2] File doesn't exist\n");
    return -1;
  }

  kprintf("Copying fd %d; name: %s\n", fd, file->name);

  File *file2 = running->fd_table[new_fd];

  if (file2) {
    file2->fs->close(file2);
  }

  // refer to the same file
  running->fd_table[new_fd] = file;
  kprintf("New fd %d name: %s\n", new_fd, running->fd_table[new_fd]->name);

  return new_fd;
}

int sys_readdir(int handle, DirectoryEntry *buffer, size_t max_size) {
  kprintf("Dirent buffer is at %x\n", buffer);

  if (max_size < sizeof(DirectoryEntry))
    return -1;

  extern ProcessControlBlock *running;

  File *file = running->fd_table[handle];
  if (!file) {
    kprintf("Invalid open stream\n");
    return -1;
  }

  kprintf("Reading entries from %s\n", file->name);
  DirectoryEntry *entry = vfs_readdir(file);
  if (entry) {
    *buffer = *entry;
    return 0;
  }

  return 0;
}

int sys_fcntl(int fd, int request, va_list args) {

  extern ProcessControlBlock *running;
  switch (request) {
  case F_SETFD: {
    kprintf("F_SETFD\n");
    if (valid_fd(fd)) {
      File *fp = running->fd_table[fd];
      if (fp) {
        int flags = va_arg(args, int);
        fp->mode |= flags;
        return 0;
      }
    }
    return -1;
  }
  case F_GETFD: {
    kprintf("F_GETFD\n");
    if (valid_fd(fd)) {
      File *fp = running->fd_table[fd];
      if (fp) {
        return fp->mode;
      }
    }
    break;
  }
  case F_GETFL: {
    if (valid_fd(fd)) {
      File *fp = running->fd_table[fd];
      if (fp) {
        return fp->status;
      }
    }
    return -1;
    break;
  }
  case F_SETFL: {
    kprintf("F_SETFL\n");
    if (valid_fd(fd)) {
      File *fp = running->fd_table[fd];
      if (fp) {
        int status = va_arg(args, int);
        fp->status = status;
        return 0;
      }
    }
    return -1;
    break;
  }
  default: {
    kprintf("unknown request\n");
    return -1;
  }
  }

  return 0;
}

int sys_poll(struct pollfd *fds, uint32_t count, int timeout) {
  int events = 0;
  // kprintf("[POLL] pollfd ptr %x; count %u; Timeout %d;\n", fds, count,
  // timeout);
  //  forget timeout, just loop forever
  for (uint32_t i = 0; i < count; i++) {
    int fd = fds[i].fd;
    if (valid_fd(fd)) {
      extern ProcessControlBlock *running;
      struct file *file = running->fd_table[fd];
      if (!file->fs->poll)
        kprintf("Poll is not implemented for %s :(\n", file->name);
      else {
        events += file->fs->poll(file, &fds[i], timeout);
      }
    } else {
      kprintf("[POLL]   Invalid fd %d\n", fd);
    }
  }

  return events;
}

void syscall_dispatcher(Registers *regs) {

#ifdef SYSCALL_DEBUG
  dump_regs(regs);
#endif

  u64 syscall = regs->rsi;
  u64 ret = 0;

  switch (syscall) {
  case SYS_EXIT: {
    kprintf("[SYS]  EXIT CALLED\n");
    register int status asm("r9");
    sys_exit();
    break;
  }
  case SYS_OPEN: {
    kprintf("[SYS]  OPEN CALLED\n");

    char *fp = (char *)regs->r8;
    int flags = regs->r9;

    int fd = sys_open(fp, flags);
    if (fd != -1) {
      kprintf("Got FD #%d\n", fd);
      regs->r15 = fd;
    } else {
      kprintf("Couldn't open file\n");
      regs->r15 = -1;
    }

    break;
  }
  case SYS_CLOSE: {
    kprintf("[SYS]  CLOSE CALLED\n");
    regs->r15 = sys_close(regs->r8);
    break;
  }
  case SYS_READ: {
    kprintf("[SYS]  READ CALLED\n");

    int fd = regs->r8;
    char *buf = (char *)regs->r9;
    size_t count = regs->r10;

    regs->r15 = sys_read(fd, buf, count);

    break;
  }
  case SYS_WRITE: {
    // kprintf("[SYS]  WRITE CALLED\n");
    int file = regs->r8;
    char *ptr = (char *)regs->r9;
    int len = regs->r10;

    regs->r15 = sys_write(file, ptr, len);

    break;
  }
  case SYS_LOG_LIBC: {
    register const char *msg asm("r8");
    sys_log_libc(msg);
    break;
  }
  case SYS_VM_MAP: {
    kprintf("[SYS]  VM_MAP CALLED\n");
    void *addr = (void *)regs->r8;
    size_t size = regs->r9;
    int prot = regs->r10;
    int flags = regs->r12;
    int fd = regs->r13;
    off_t off = regs->r14;

    extern ProcessControlBlock *running;
    void *ret = sys_vm_map(running, addr, size, prot, flags, fd, off);
    regs->r15 = (u64)ret;

    break;
  }
  case SYS_SEEK: {
    kprintf("[SYS]  SEEK CALLED\n");
    int fd = regs->r8;
    off_t off = regs->r9;
    int whence = regs->r10;
    regs->r15 = sys_seek(fd, off, whence);
    kprintf("Returning offset %d\n", regs->r15);

    break;
  }
  case SYS_TCB_SET: {
    kprintf("[SYS]  TCB_SET CALLED\n");
    void *ptr = (void *)regs->r8;
    regs->r15 = sys_tcb_set(ptr);
    break;
  }
  case SYS_IOCTL: {
    kprintf("[SYS]  IOCTL CALLED\n");
    int fd = regs->r8;
    unsigned long req = regs->r9;
    void *arg = (void *)regs->r10;

    regs->r15 = sys_ioctl(fd, req, arg);
    break;
  }
  case SYS_STAT: {
    kprintf("[SYS]  STAT CALLED\n");
    const char *path = (const char *)regs->r8;
    VfsNodeStat *statbuf = (VfsNodeStat *)regs->r9;

    regs->r15 = sys_stat(path, statbuf);
    break;
  }
  case SYS_FSTAT: {
    kprintf("[SYS]  FSTAT CALLED\n");
    int fd = regs->r8;
    VfsNodeStat *statbuf = (VfsNodeStat *)regs->r9;

    regs->r15 = sys_fstat(fd, statbuf);
    break;
  }
  case SYS_GETPID: {
    regs->r15 = sys_getpid();
    break;
  }
  case SYS_FORK: {
    kprintf("[SYS]  FORK CALLED\n");
    regs->r15 = sys_fork(regs);
    break;
  }
  case SYS_FCNTL: {
    kprintf("[SYS]  FCNTL CALLED\n");
    regs->r15 = sys_fcntl(regs->r8, regs->r9, (void *)regs->r10);
    break;
  }
  case SYS_POLL: {
    kprintf("[SYS]  POLL CALLED\n");
    regs->r15 = sys_poll((struct pollfd *)regs->r8, regs->r9, regs->r10);
    break;
  }
  case SYS_DUP: {
    regs->r15 = sys_dup(regs->r8, regs->r9);
    break;
  }
  case SYS_DUP2: {
    regs->r15 = sys_dup2(regs->r8, regs->r9, regs->r10);
    break;
  }
  case SYS_READDIR: {
    regs->r15 = sys_readdir(regs->r8, (DirectoryEntry *)regs->r9, regs->r10);
    break;
  }
  case SYS_EXEC: {
    regs->r15 =
        sys_execve((char *)regs->r8, (char **)regs->r9, (char **)regs->r10);
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
