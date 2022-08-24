#include <stdbool.h>
#include <stdint.h>
#include <string/string.h>
#include <unistd.h>
#include <util.h>

void cli() { __asm__("cli"); }
void sti() { __asm__("sti"); }

i32 abs(i32 val) {
  if (val < 0)
    return -val;
  else
    return val;
}

i64 ll_abs(i64 val) {
  if (val < 0)
    return -val;
  else
    return val;
}

void swap(char *x, char *y) {
  char t = *x;
  *x = *y;
  *y = t;
}

char *reverse(char *buf, int i, int j) {
  while (i < j)
    swap(&buf[i++], &buf[j--]);

  return buf;
}

char *ulltoa(u64 val, char *buf, u8 base) {
  // invalid input
  if (base < 2 || base > 32)
    return buf;

  u64 n = val;
  u64 i = 0;

  while (n) {
    char r = n % base;
    if (r >= 10)
      buf[i++] = 65 + (r - 10);
    else
      buf[i++] = 48 + r;
    n = n / base;
  }

  // if number is 0
  if (i == 0)
    buf[i++] = '0';

  buf[i] = '\0'; // null terminate string

  // reverse the string and return it
  return reverse(buf, 0, i - 1);
}

char *lltoa(i64 value, char *buf, u8 base) {
  // invalid input
  if (base < 2 || base > 32)
    return buf;

  i64 n = ll_abs(value);

  u64 i = 0;
  while (n) {
    char r = n % base;
    if (r >= 10)
      buf[i++] = 65 + (r - 10);
    else
      buf[i++] = 48 + r;
    n = n / base;
  }

  // if number is 0
  if (i == 0)
    buf[i++] = '0';

  // If base is 10 and value is negative, the resulting string
  // is preceded with a minus sign (-)
  // With any other base, value is always considered unsigned
  if (value < 0 && base == 10)
    buf[i++] = '-';

  buf[i] = '\0'; // null terminate string

  // reverse the string and return it
  return reverse(buf, 0, i - 1);
}

char *itoa(i32 value, char *buf, u8 base) {
  // invalid input
  if (base < 2 || base > 32)
    return buf;

  // consider absolute value of number
  i64 n = abs(value);

  i64 i = 0;
  while (n) {
    i32 r = n % base;
    if (r >= 10)
      buf[i++] = 65 + (r - 10);
    else
      buf[i++] = 48 + r;
    n = n / base;
  }

  // if number is 0
  if (i == 0)
    buf[i++] = '0';

  // If base is 10 and value is negative, the resulting string
  // is preceded with a minus sign (-)
  // With any other base, value is always considered unsigned
  if (value < 0 && base == 10)
    buf[i++] = '-';

  buf[i] = '\0'; // null terminate string

  // reverse the string and return it
  return reverse(buf, 0, i - 1);
}

void set_bit(u8 *word, u8 bit) {
  *word |= (1 << bit);
  return;
}

void clear_bit(u8 *word, u8 bit) {
  *word &= ~(1 << bit);
  return;
}

bool check_bit(u8 *word, u8 bit) { return *word & (1 << bit); }

bool starts_with(const char *a, const char *b) {
  size_t alen = strlen(a);
  size_t blen = strlen(b);

  if (alen < blen) {
    return false;
  }

  int i = 0;
  while (i < blen) {
    if (a[i] != b[i])
      return false;
    i++;
  }

  return true;
}
