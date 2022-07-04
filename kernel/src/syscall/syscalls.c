#include <stdint.h>

#include <config.h>
#include <cpu/cpu.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <proc/proc.h>
#include <syscall/syscalls.h>
#include <typedefs.h>

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
  kprintf("Got file %s with size %d bytes\n", file->name, file->size);

  if (file == NULL)
    return -1;

  extern ProcessControlBlock *gp_current_process;

  // error on overflow
  if (gp_current_process->fd_length > MAX_PROC_FDS)
    return -1;

  int fd = map_file_to_proc(gp_current_process, file);

  return fd;
}

int sys_close(int fd) {
  vmm_switch_page_directory(kernel_cr3);
  extern ProcessControlBlock *gp_current_process;
  unmap_fd_from_proc(gp_current_process, fd);
  return 0;
}

int sys_read(int file, char *ptr, size_t len) {
  extern ProcessControlBlock *gp_current_process;
  struct file *f = gp_current_process->fd_table[file];
  vfs_dump();
  // kprintf("File ptr %x. Name %s. Device %x\n", f, f->name, f->device);
  int bytes_read = vfs_read(f, (u8 *)ptr, f->position, len);

  return bytes_read;
}

int sys_write(int file, char *ptr, int len) {
#ifdef SYSCALL_DEBUG
  kprintf("Called SYS_WRITE File: %d Buffer addr: 0x%x Length: %d\n", file, ptr,
          len);
#endif
  for (int i = 0; i < len; ++i)
    kprintf("%c", ptr[i]);
  return len;
}

/* snatched from lemon/vm-flags.h */
#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_PRIVATE 0x01
#define MAP_SHARED 0x02
#define MAP_FIXED 0x04
#define MAP_ANON 0x08
#define MAP_ANONYMOUS 0x08
#define MAP_NORESERVE 0x10

void *sys_vm_map(void *addr, size_t size, int prot, int flags, int fd,
                 off_t offset) {

  kprintf("[MMAP] Requesting %llu bytes\n", size);
  kprintf("[MMAP] Hint : 0x%llx\n", addr);
  kprintf("[MMAP] Size : 0x%llx\n", size);

  extern ProcessControlBlock *gp_current_process;

  kprintf("Current process at 0x%x\n", gp_current_process);

  if (!(flags & MAP_ANON)) {
    kprintf("[MMAP] Non anonymous mapping\n");
  } else {
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
    if (flags & MAP_FIXED) {
      virt_base = addr;
    } else {
      virt_base = (void *)gp_current_process->mmap_base;
      gp_current_process->mmap_base += size;
    }

    kprintf("[MMAP] Found free chunk at 0x%x phys\n", phys_base);
    int page_flags = PAGE_USER | PAGE_PRESENT | PAGE_WRITE;

    if (flags & PROT_WRITE)
      page_flags |= PAGE_WRITE;

    kprintf("Virt base is %x\n", virt_base);

    vmm_map_range(
        (void *)((u64)gp_current_process->cr3 + PAGING_VIRTUAL_OFFSET),
        virt_base, phys_base, size, page_flags);

    // vmm_switch_page_directory(gp_current_process->cr3);

    kprintf("[MMAP] Returning 0x%x\n", virt_base);

    return virt_base;
  }

  kprintf("Returning NULL");
  return NULL;
}

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

off_t sys_seek(int fd, off_t offset, int whence) {
  extern ProcessControlBlock *gp_current_process;
  kprintf("[SYS_SEEK] FD is %d. Offset is %d. Whence is %d\n", fd, offset,
          whence);

  switch (whence) {
  case SEEK_CUR:
    kprintf("[SYS_SEEK] whence is SEEK_CUR\n");
    gp_current_process->fd_table[fd]->position += offset;
    break;
  case SEEK_SET:
    kprintf("[SYS_SEEK] whence is SEEK_SET\n");
    gp_current_process->fd_table[fd]->position = offset;
    break;
  case SEEK_END:
    kprintf("[SYS_SEEK] whence is SEEK_END\n");
    gp_current_process->fd_table[fd]->position =
        gp_current_process->fd_table[fd]->size + offset;
    break;
  default:
    kprintf("[SYS_SEEK] Whence is none\n");
    break;
  }

  return gp_current_process->fd_table[fd]->position;
}

int sys_tcb_set(void *ptr) {
  wrmsr(FSBASE, (uint64_t)ptr);
  return 0;
}

int sys_execve(char *name, char **argv, char **env) { return -1; }

int sys_fork() { return -1; }

void syscall_dispatcher(Registers regs) {

#ifdef SYSCALL_DEBUG
  dump_regs(&regs);
#endif

  u64 syscall = regs.rsi;
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

    char *fp = (char *)regs.r8;
    int flags = regs.r9;

    int fd = sys_open(fp, flags);
    if (fd != -1) {
      kprintf("Got FD #%d\n", fd);
      regs.r15 = fd;
    } else {
      kprintf("Couldn't open file\n");
      for (;;)
        ;
    }

    break;
  }
  case SYS_CLOSE: {
    kprintf("[SYS]  CLOSE CALLED\n");
    break;
  }
  case SYS_READ: {
    kprintf("[SYS]  READ CALLED\n");

    int fd = regs.r8;
    char *buf = (char *)regs.r9;
    size_t count = regs.r10;

    regs.r15 = sys_read(fd, buf, count);

    break;
  }
  case SYS_WRITE: {
    // kprintf("[SYS]  WRITE CALLED\n");
    int file = regs.r8;
    char *ptr = (char *)regs.r9;
    int len = regs.r10;

    regs.r15 = sys_write(file, ptr, len);

    break;
  }
  case SYS_LOG_LIBC: {
    register const char *msg asm("r8");
    sys_log_libc(msg);
    break;
  }
  case SYS_VM_MAP: {
    kprintf("[SYS]  VM_MAP CALLED\n");
    void *addr = (void *)regs.r8;
    size_t size = regs.r9;
    int prot = regs.r10;
    int flags = regs.r12;
    int fd = regs.r13;
    off_t off = regs.r14;

    void *ret = sys_vm_map(addr, size, prot, flags, fd, off);
    regs.r15 = (u64)ret;

    break;
  }
  case SYS_SEEK: {
    kprintf("[SYS]  SEEK CALLED\n");
    int fd = regs.r8;
    off_t off = regs.r9;
    int whence = regs.r10;
    regs.r15 = sys_seek(fd, off, whence);
    kprintf("Returning offset %llu\n", regs.r15);

    break;
  }
  case SYS_TCB_SET: {
    kprintf("[SYS]  TCB_SET CALLED\n");
    void *ptr = (void *)regs.r8;
    regs.r15 = sys_tcb_set(ptr);
    break;
  }
  default: {
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
