#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>
#include <util.h>

#include <libc/abi-bits/fcntl.h>
#include <libc/asm/ioctls.h>

#define PTY_BUF_SIZE 0x1000
#define MAX_PTYS 8 // for pts/X (x is only one digit)

struct file *pty_open(const char *filename, int flags);
uint64_t pty_write(struct file *file, size_t size, u8 *buffer);
uint64_t pty_read(struct file *file, size_t size, u8 *buffer);
void pty_close(struct file *f);
int pty_ioctl(struct file *, uint32_t request, void *arg);
int pty_poll(struct file *, struct pollfd *fd);

FileSystem ptyfs = {.open = pty_open,
                    .read = pty_read,
                    .write = pty_write,
                    .close = pty_close,
                    .ioctl = pty_ioctl,
                    .poll = pty_poll};

struct fs *ptydev = &ptyfs;

struct pty {
  int id;

  File *master;
  File *slave;

  char *master_buf;
  char *slave_buf;

  size_t master_buf_size;
  size_t slave_buf_size;

  int master_lock;
  int slave_lock;
};

static struct pty g_ptys[MAX_PTYS];
static int g_pty_counter = 0;

struct file *pty_open(const char *filename, int flags) {
  kprintf("[PTY]  opening %s\n", filename);
  if (strcmp(filename, "ptmx") == 0) {
    // create new master/slave pair

    if (g_pty_counter >= MAX_PTYS) {
      kprintf("Too many pty pairs\n");
      for (;;)
        ;
    } else {

      struct pty new_pty = (struct pty){.id = g_pty_counter,
                                        .master = NULL,
                                        .slave = NULL,
                                        .master_buf = kmalloc(PTY_BUF_SIZE),
                                        .slave_buf = kmalloc(PTY_BUF_SIZE),
                                        .master_buf_size = PTY_BUF_SIZE,
                                        .slave_buf_size = PTY_BUF_SIZE,
                                        .master_lock = 0,
                                        .slave_lock = 0};

      File *master_file = kmalloc(sizeof(File));
      master_file->name = kmalloc(strlen("ptm/XYZ\0"));
      memset(master_file->name, 0, strlen("ptm/XYZ\0"));
      strcpy(master_file->name, "ptm/");
      master_file->name[4] = '0' + g_pty_counter;
      master_file->name[5] = '\0';

      master_file->size = PTY_BUF_SIZE;
      master_file->position = 0;
      master_file->fs = ptydev;
      master_file->inode = g_pty_counter;
      master_file->device = MKDEV(5, 2);

      File *slave_file = kmalloc(sizeof(File));
      slave_file->name = kmalloc(strlen("pts/XYZ\0"));

      // asprintf would be so nice here
      memset(slave_file->name, 0, strlen("pts/XYZ\0"));
      strcpy(slave_file->name, "pts/");
      slave_file->name[4] = '0' + g_pty_counter;
      slave_file->name[5] = '\0';

      slave_file->size = PTY_BUF_SIZE;
      slave_file->position = 0;
      slave_file->fs = ptydev;
      slave_file->device = MKDEV(136, g_pty_counter);

      new_pty.master = master_file;
      new_pty.slave = slave_file;

      g_ptys[g_pty_counter] = new_pty;
      ++g_pty_counter;

      kprintf("[PTY]    Created master/slave pair\n");
      return new_pty.master;
    };

  } else if (starts_with(filename, "pts/")) {
    // check if it's trying to open `/dev/pts/XYZ`
    // this works for now bc it only goes up to 8
    int id = filename[4] - '0';
    kprintf("[PTY]  pts id is %d\n", id);
    return g_ptys[id].slave;
  }

  return NULL;
}

uint64_t pty_write(struct file *file, size_t size, u8 *buffer) { return -1; }
uint64_t pty_read(struct file *file, size_t size, u8 *buffer) { return -1; }

void pty_close(struct file *file) { return; }

int pty_poll(struct file *file, struct pollfd *fd) { return 0; }

int pty_ioctl(struct file *file, uint32_t request, void *arg) {
  if (MAJOR(file->device) == 136) {
    kprintf("Calling ioctl on slave\n");
    for (;;)
      ;
  }
  switch (request) {
  case TIOCSPTLCK: {
    int *lock = (int *)arg;
    g_ptys[file->inode].slave_lock = *lock;
    break;
  }
  case TIOCGPTN: {
    int *ptn = (int *)arg;
    *ptn = MINOR(g_ptys[file->inode].slave->device);
    break;
  }
  default:
    kprintf("Unknown ioctl request %lu\n", request);
    for (;;)
      ;
  }

  return 0;
}
