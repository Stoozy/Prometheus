#include <drivers/tty.h>
#include <fs/vfs.h>
#include <kmalloc.h>
#include <kprintf.h>
#include <string/string.h>
#include <util.h>

#include <libc/abi-bits/fcntl.h>
#include <libc/asm/ioctls.h>

#define PTY_BUF_SIZE 0x1000
#define MAX_PTYS 8 // for pts/X (x is only one digit)

struct ptm *g_ptm = NULL;

struct file *ptmx_open(const char *filename, int flags);

int ptm_ioctl(struct file *, uint32_t request, void *arg);
int ptm_poll(struct file *, struct pollfd *fd);

int pts_ioctl(struct file *, uint32_t request, void *arg);
int pts_poll(struct file *, struct pollfd *fd);

uint64_t ptm_write(struct file *file, size_t size, u8 *buffer);
uint64_t pts_write(struct file *file, size_t size, u8 *buffer);

uint64_t ptm_read(struct file *file, size_t size, u8 *buffer);
uint64_t pts_read(struct file *file, size_t size, u8 *buffer);

void ptm_close(struct file *file);
void pts_close(struct file *file);

FileSystem ptmx_ops = {.open = ptmx_open, .close = NULL};

FileSystem pts_ops = {.read = pts_read, // these depend on master/slave
                      .write = pts_write,
                      .close = pts_close,
                      .ioctl = pts_ioctl,
                      .poll = pts_poll};

FileSystem ptm_ops = {.read = ptm_read, // these depend on master/slave
                      .write = ptm_write,
                      .close = ptm_close,
                      .ioctl = ptm_ioctl,
                      .poll = ptm_poll};

// just needed for open so it doesn't matter which one is used
struct fs *ptydev = &ptmx_ops;

struct ptm g_ptys[MAX_PTYS];
static int g_pty_counter = 1;

struct file *ptmx_open(const char *filename, int flags) {
  if (starts_with(filename, "ptmx")) {
    kprintf("[PTMX]  opening %s\n", filename);

    struct ptm new_master = {
        .inlock = 0, .buffer = NULL, .bufsize = TTY_BUF_SIZE, .slave = NULL};
    new_master.buffer = kmalloc(TTY_BUF_SIZE);
    new_master.bufsize = TTY_BUF_SIZE;

    struct tty *slave_tty = kmalloc(sizeof(Tty));
    memset(slave_tty, 0, sizeof(Tty));

    slave_tty->in = slave_tty->out = false;
    slave_tty->in_size = slave_tty->out_size = TTY_BUF_SIZE;

    slave_tty->buffer_in = kmalloc(TTY_BUF_SIZE);
    slave_tty->buffer_out = kmalloc(TTY_BUF_SIZE);

    struct pts slave = {.id = g_pty_counter,
                        .tty = slave_tty,
                        .master = &new_master,
                        .ws = {80, 80, 10, 10}};

    struct pts *new_slave = kmalloc(sizeof(struct pts));
    *new_slave = slave;

    new_master.slave = new_slave;
    g_ptys[g_pty_counter] = new_master;
    g_ptm = &g_ptys[g_pty_counter];

    File *master_file = kmalloc(sizeof(File));

    master_file->name = kmalloc(strlen("ptm/X") + 1);
    strcpy(master_file->name, "ptm/X\0");
    master_file->name[4] = '0' + g_pty_counter;

    master_file->fs = &ptm_ops;
    master_file->private_data = &g_ptys[g_pty_counter];
    master_file->position = 0;
    master_file->type = VFS_CHARDEVICE;

    return master_file;
  } else if (starts_with(filename, "pts/")) {
    // counter only goes up to 8 so this
    // should be fine for now
    int id = filename[4] - '0';
    kprintf("[PTS]    opening pts/%d\n", id);

    struct ptm master = g_ptys[id];

    File *slave_file = kmalloc(sizeof(File));

    slave_file->name = kmalloc(strlen("pts/X") + 1);
    strcpy(slave_file->name, "pts/X");
    slave_file->name[4] = '0' + g_pty_counter;
    slave_file->name[5] = '\0';

    slave_file->fs = &pts_ops;
    slave_file->position = 0;
    slave_file->private_data = master.slave;
    slave_file->type = VFS_CHARDEVICE;

    g_pty_counter++;
    return slave_file;
  }
  return NULL;
}

uint64_t ptm_write(struct file *file, size_t size, u8 *buffer) {
  struct ptm *master = file->private_data;

  kprintf("[PTM]  Writing to  ptm/%d\n", master->slave->id);
  memcpy(master->buffer + file->position, buffer, size);

  return size;
}

uint64_t pts_write(struct file *file, size_t size, u8 *buffer) {
  struct pts *slave = file->private_data;
  if (slave->tty->in_size < size) {
    kprintf("[PTS]  Writing to  pts/%d\n", slave->id);
    kprintf("Data: ");
    for (int i = 0; i < slave->tty->in_size; i++)
      kprintf("%c ", buffer[i]);
    kprintf("\n");

    memcpy(slave->tty->buffer_in + file->position, buffer, slave->tty->in_size);
    slave->tty->in = false;

    // write to master as well
    memcpy(slave->master->buffer, buffer, slave->tty->in_size);
    slave->master->in = true;

    return slave->tty->in_size;
  }

  kprintf("[PTS]  Writing to  pts/%d\n", slave->id);
  kprintf("Data: ");
  for (int i = 0; i < size; i++)
    kprintf("%c ", buffer[i]);
  kprintf("\n");

  memcpy(slave->tty->buffer_in + file->position, buffer, size);
  slave->tty->in = false;

  // write to master as well
  memcpy(slave->master->buffer, buffer, size);
  slave->master->in = true;

  asm("sti");
  for (;;)
    ;

  return size;
}

uint64_t ptm_read(struct file *file, size_t size, u8 *buffer) {
  struct ptm *master = file->private_data;

  kprintf("[PTM]  Reading from  ptm/%d\n", master->slave->id);
  if (size > master->bufsize) {
    memcpy(buffer, master->buffer, master->bufsize);
    master->in = false;

    memset(master->buffer, 0, master->bufsize);
    return master->bufsize;
  } else {
    memcpy(buffer, master->buffer, size);
    master->in = false;
    memset(master->buffer, 0, master->bufsize);
    return size;
  }
}

uint64_t pts_read(struct file *file, size_t size, u8 *buffer) {
  struct pts *slave = file->private_data;

  kprintf("[PTS]  Reading from  pts/%d\n", slave->id);
  memcpy(buffer, slave->tty->buffer_in + file->position, size);

  memset(slave->tty->buffer_in, 0, size);
  slave->tty->in = false;

  return size;
}

int ptm_ioctl(struct file *file, uint32_t request, void *arg) {
  kprintf("[PTM]  ioctl() \n");

  struct ptm *master = file->private_data;

  switch (request) {
  case TIOCGPTN: {
    int *ptn = (int *)arg;
    *ptn = master->slave->id;
    break;
  }
  case TIOCSPTLCK: {
    int *lock = (int *)arg;
    master->slave->tty->inlock = *lock;
    break;
  }
  default:
    kprintf("[PTM]   Unknown request %d\n", request);

    for (;;)
      ;
    break;
  }

  return 0;
}

int pts_ioctl(struct file *file, uint32_t request, void *arg) {
  kprintf("[PTS]  ioctl() \n");

  struct pts *slave = file->private_data;
  switch (request) {

  case TIOCGWINSZ: {
    struct winsize *ws = (struct winsize *)arg;
    *ws = slave->ws;
    break;
  }
  case TIOCSCTTY: {
    extern Tty *gp_active_tty;
    gp_active_tty = slave->tty;
    //  for (;;)
    //  kprintf("Set pts/%d as active tty\n", slave->tty->id);
    break;
  }
  default:
    kprintf("[PTS]   Unknown request %d\n", request);
    for (;;)
      ;
    break;
  }

  return 0;
}

int ptm_poll(struct file *file, struct pollfd *pfd) {
  struct ptm *master = file->private_data;
  kprintf("Called poll on ptm/%d\n", master->slave->id);
  kprintf("master buffer size %d\n", master->bufsize);
  int events = 0;
  switch (pfd->events) {
  case POLLIN: {
    if (master->in) {
      pfd->revents |= POLLIN;
      events++;
      kprintf("Got POLLIN event\n");
    }
    break;
  }
  case POLLOUT: {
    break;
  }
  }

  return events;
}

int pts_poll(struct file *file, struct pollfd *pfd) {
  struct pts *slave = file->private_data;
  kprintf("Called poll on pts/%d\n", slave->id);

  int events = 0;
  switch (pfd->events) {
  case POLLIN: {
    if (slave->tty->in) {
      pfd->revents |= POLLIN;
      events++;
      slave->tty->in = false;
    }
    break;
  }
  case POLLOUT: {
    if (slave->tty->out) {
      pfd->revents |= POLLOUT;
      events++;
      slave->tty->out = false;
    }
    break;
  }
  }

  for (int i = 0; i < 100000; i++)
    asm("nop");
  return events;
}

// TODO
void ptm_close(struct file *file) { return; }
void pts_close(struct file *file) { return; }
