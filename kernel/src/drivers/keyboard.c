#include <drivers/keyboard.h>
#include <drivers/tty.h>
#include <fs/vfs.h>
#include <kprintf.h>
#include <proc/proc.h>
#include <string/string.h>

static bool shift_down = false;
static bool alt_down = false;
static bool ctrl_down = false;

unsigned char kbdmix[128] = {
    0,    27,  '1', '2',        '3',  '4', '5', '6', '7',  '8', /* 9 */
    '9',  '0', '+', /*'´' */ 0, '\b',                           /* Backspace */
    '\t',                                                       /* Tab */
    'q',  'w', 'e', 'r',                                        /* 19 */
    't',  'y', 'u', 'i',        'o',  'p', '[', ']', '\n',      /* Enter key */
    0, /* 29   - Control */
    'a',  's', 'd', 'f',        'g',  'h', 'j', 'k', 'l',  ';', /* 39 */
    '\'', '<', 0,                                               /* Left shift */
    '\\', 'z', 'x', 'c',        'v',  'b', 'n',                 /* 49 */
    'm',  ',', '.', '-',        0,                      /* Right shift */
    '*',  0,                                            /* Alt */
    ' ',                                                /* Space bar */
    0,                                                  /* Caps lock */
    0,                                                  /* 59 - F1 key ... > */
    0,    0,   0,   0,          0,    0,   0,   0,   0, /* < ... F10 */
    0,                                                  /* 69 - Num lock*/
    0,                                                  /* Scroll Lock */
    0,                                                  /* Home key */
    0,                                                  /* Up Arrow */
    0,                                                  /* Page Up */
    '-',  0,                                            /* Left Arrow */
    0,    0,                                            /* Right Arrow */
    '+',  0,                                            /* 79 - End key*/
    0,                                                  /* Down Arrow */
    0,                                                  /* Page Down */
    0,                                                  /* Insert Key */
    0,                                                  /* Delete Key */
    0,    0,   '<', 0,                                  /* F11 Key */
    0,                                                  /* F12 Key */
    0, /* All other keys are undefined */
};
unsigned char kbdse_shift[128] = {
    0,    27,  '!',  '\"', '#',  0 /* shift+4 */,
    '%',  '&', '/',  '(',        /* 9 */
    ')',  '=', '?',  '`',  '\b', /* Backspace */
    '\t',                        /* Tab */

    'Q',  'W', 'E',  'R', /* 19 */
    'T',  'Y', 'U',  'I',  'O',  'P',
    'A',  'A', '\n', /* Enter key */
    0,               /* 29   - Control */
    'A',  'S', 'D',  'F',  'G',  'H',
    'J',  'K', 'L',  'O', /* 39 */
    '\'', '>', 0,         /* Left shift */
    '*',  'Z', 'X',  'C',  'V',  'B',
    'N',                      /* 49 */
    'M',  ';', ':',  '_',  0, /* Right shift */

    '*',  0, /* Alt */
    ' ',     /* Space bar */
    0,       /* Caps lock */
    0,       /* 59 - F1 key ... > */
    0,    0,   0,    0,    0,    0,
    0,    0,   0,       /* < ... F10 */
    0,                  /* 69 - Num lock*/
    0,                  /* Scroll Lock */
    0,                  /* Home key */
    0,                  /* Up Arrow */
    0,                  /* Page Up */
    '-',  0,            /* Left Arrow */
    0,    0,            /* Right Arrow */
    '+',  0,            /* 79 - End key*/
    0,                  /* Down Arrow */
    0,                  /* Page Down */
    0,                  /* Insert Key */
    0,                  /* Delete Key */
    0,    0,   '>',  0, /* F11 Key */
    0,                  /* F12 Key */
    0,                  /* All other keys are undefined */
};

unsigned char kbdse_alt[128] = {
    0,    27,  0 /*alt+1*/, '@', 0,    '$', 0,   0,   '{',  '[', /* 9 */
    ']',  '}', '\\',        '=', '\b',                           /* Backspace */
    '\t',                                                        /* Tab */
    'q',  'w', 'e',         'r',                                 /* 19 */
    't',  'y', 'u',         'i', 'o',  'p', '[', ']', '\n',      /* Enter key */
    0, /* 29   - Control */
    'a',  's', 'd',         'f', 'g',  'h', 'j', 'k', 'l',  ';', /* 39 */
    '\'', '`', 0,                                        /* Left shift */
    '\\', 'z', 'x',         'c', 'v',  'b', 'n',         /* 49 */
    'm',  ',', '.',         '/', 0,                      /* Right shift */
    '*',  0,                                             /* Alt */
    ' ',                                                 /* Space bar */
    0,                                                   /* Caps lock */
    0,                                                   /* 59 - F1 key ... > */
    0,    0,   0,           0,   0,    0,   0,   0,   0, /* < ... F10 */
    0,                                                   /* 69 - Num lock*/
    0,                                                   /* Scroll Lock */
    0,                                                   /* Home key */
    0,                                                   /* Up Arrow */
    0,                                                   /* Page Up */
    '-',  0,                                             /* Left Arrow */
    0,    0,                                             /* Right Arrow */
    '+',  0,                                             /* 79 - End key*/
    0,                                                   /* Down Arrow */
    0,                                                   /* Page Down */
    0,                                                   /* Insert Key */
    0,                                                   /* Delete Key */
    0,    0,   '|',         0,                           /* F11 Key */
    0,                                                   /* F12 Key */
    0, /* All other keys are undefined */
};

static size_t kbd_write_to_tty(size_t size, u8 *buffer) {

  // TODO
  return 0;
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
    if (shift_down)
      kbd_write_to_tty(1, &kbdse_shift[scan_code]);
    else if (alt_down)
      kbd_write_to_tty(1, &kbdse_alt[scan_code]);
    else
      kbd_write_to_tty(1, &kbdmix[scan_code]);

    break;
  }
}
