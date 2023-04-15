#include "libk/kmalloc.h"
#include "libk/ringbuffer.h"
#include "memory/vmm.h"
#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <fs/vfs.h>
#include <libk/kprintf.h>
#include <libk/util.h>
#include <proc/proc.h>
#include <string/string.h>

#define KBD_BUFSIZE 1024

extern ProcessQueue ready_queue;

#define MAX_BUFS 12
RingBuffer *buffers[MAX_BUFS];

static bool shift_down = false;
static bool alt_down = false;
static bool ctrl_down = false;

static const char convtab_capslock[] = {
    '\0', '\e', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
    '-',  '=',  '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
    'O',  'P',  '[',  ']',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
    'J',  'K',  'L',  ';',  '\'', '`',  '\0', '\\', 'Z',  'X', 'C', 'V',
    'B',  'N',  'M',  ',',  '.',  '/',  '\0', '\0', '\0', ' '};

static const char convtab_shift[] = {

    '\0', '\e', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
    '_',  '+',  '\b', '\t', 'Q',  'W',  'E',  'R',  'T',  'Y', 'U', 'I',
    'O',  'P',  '{',  '}',  '\n', '\0', 'A',  'S',  'D',  'F', 'G', 'H',
    'J',  'K',  'L',  ':',  '"',  '~',  '\0', '|',  'Z',  'X', 'C', 'V',
    'B',  'N',  'M',  '<',  '>',  '?',  '\0', '\0', '\0', ' '};

static const char convtab_shift_capslock[] = {
    '\0', '\e', '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*', '(', ')',
    '_',  '+',  '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
    'o',  'p',  '{',  '}',  '\n', '\0', 'a',  's',  'd',  'f', 'g', 'h',
    'j',  'k',  'l',  ':',  '"',  '~',  '\0', '|',  'z',  'x', 'c', 'v',
    'b',  'n',  'm',  '<',  '>',  '?',  '\0', '\0', '\0', ' '};

static const char convtab_nomod[] = {
    '\0', '\e', '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8', '9', '0',
    '-',  '=',  '\b', '\t', 'q',  'w',  'e',  'r',  't',  'y', 'u', 'i',
    'o',  'p',  '[',  ']',  '\n', '\0', 'a',  's',  'd',  'f', 'g', 'h',
    'j',  'k',  'l',  ';',  '\'', '`',  '\0', '\\', 'z',  'x', 'c', 'v',
    'b',  'n',  'm',  ',',  '.',  '/',  '\0', '\0', '\0', ' '};

ProcessQueue kbd_wait_queue = {0, NULL, NULL};

void kbd_write_to_buffer(uint8_t c) {

  for (int i = 0; i < MAX_BUFS; i++) {
    if (buffers[i]) {
      rb_push(buffers[i], &c);
    }
  }
}

u8 kbd_read_from_buffer() {
  u8 uc;
  if (!rb_pop(buffers[0], &uc)) {

    // buffer is empty
    // block the current running process
    kprintf("No data available. Blocking current task\n");
    extern ProcessControlBlock *running;
    pqueue_push(&kbd_wait_queue, running);
    block_process(running, WAITING);

    // note: this loop is done  a few times
    // until it's time to switch tasks and once
    // more after the process wakes up at which
    // point the keyboard_buffer should be readable
    for (;;)
      if (rb_pop(buffers[0], &uc)) {
        goto done_waiting;
      }
  }

done_waiting:
  return uc;
}

void handle_scan(u8 scan_code) {
  // TODO check for key release, and then write to tty

  switch (scan_code) {
  case 0x2a: /* shift down */
  case 0x36: /* right shift down */
    shift_down = true;
    break;
  case 0xaa: /* shift up */
  case 0xb6: /* right shift up */
    shift_down = false;
    break;
  case 0x1d: /* ctrl down */
    ctrl_down = true;
    break;
  case 0x9d: /* ctrl up */
    ctrl_down = false;
    break;

  case 0x38: /* alt down */
    alt_down = true;
    break;
  case 0xb8: /* alt up */
    alt_down = false;
    break;
  default:
    if (scan_code & 0x80)
      break;

    char c;

    if (shift_down)
      c = convtab_shift[scan_code];
    else if (ctrl_down) {
      c = toupper(convtab_nomod[scan_code]) - 0x40;
    } else
      c = convtab_nomod[scan_code];

    kbd_write_to_buffer(c);

    break;
  }
}

void kbd_register_buffer(RingBuffer *buffer) {
  for (int i = 0; i < MAX_BUFS; i++) {
    if (!buffers[i]) {
      buffers[i] = buffer;
      return;
    }
  }
}

void kbd_init() { rb_init(buffers[0], KBD_BUFSIZE, sizeof(u8)); }
