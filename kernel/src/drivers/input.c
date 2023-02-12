// #include "drivers/keyboard.h"
// #include "fs/devfs.h"
// #include "libk/ringbuffer.h"
// #include <fs/vfs.h>
// #include <libk/kmalloc.h>
// #include <libk/kprintf.h>
// #include <libk/ringbuffer.h>
// #include <string/string.h>

// File *input_open(const char *filename, int flags);
// void input_close(struct file *);
// size_t input_read(struct file *, size_t, uint8_t *buf);
// int input_poll(struct file *, struct pollfd *, int timeout);

// #define INPUT_BUFSIZE 4096

// FileSystem input_fs = {.open = input_open,
//                        .close = input_close,
//                        .read = input_read,
//                        .poll = input_poll};

// struct input_dev {
//   char name[256];
//   RingBuffer buffer;
// };

// void input_init() {
//   CharacterDevice *chardev = kmalloc(sizeof(CharacterDevice));
//   chardev->dev = MKDEV(13, 64);
//   chardev->fs = &input_fs;
//   chardev->name = kmalloc(256);
//   sprintf(chardev->name, "input/event0");

//   devfs_register_chardev(chardev);
// }

// File *input_open(const char *filename, int flags) {
//   if (strcmp(filename, "input/event0") != 0)
//     return NULL;

//   struct input_dev *idev = kmalloc(sizeof(struct input_dev));
//   rb_init(&idev->buffer, INPUT_BUFSIZE, sizeof(char));

//   kbd_register_buffer(&idev->buffer);

//   File *file = kmalloc(sizeof(File));
//   file->name = kmalloc(256);
//   sprintf(file->name, "input/event0");

//   file->device = MKDEV(13, 64);
//   file->fs = &input_fs;
//   file->private_data = idev;

//   return file;
// }

// void input_close(struct file *file) {
//   for (;;)
//     kprintf("TODO: input_close\n");
// }

// size_t input_read(struct file *file, size_t size, uint8_t *buf) {
//   struct input_dev *idev = file->private_data;

//   char ch;
//   size_t ret = 0;
//   while (rb_pop(&idev->buffer, &ch)) {
//     buf[ret++] = ch;
//   }

//   return ret;
// }

// int input_poll(struct file *file, struct pollfd *pfd, int timeout) {
//   struct input_dev *idev = file->private_data;

//   int evt = 0;

//   if (pfd->events & POLLIN) {
//     if (idev->buffer.len) {
//       pfd->revents |= POLLIN;
//       evt++;
//     }
//   }

//   if (pfd->events & POLLOUT) {
//     pfd->revents |= POLLOUT;
//     evt++;
//   }

//   return evt;
// }
