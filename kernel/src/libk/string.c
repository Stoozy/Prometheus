#include <libk/kmalloc.h>
#include <string/string.h>

int memcmp(const void *aptr, const void *bptr, size_t size) {
  const unsigned char *a = (const unsigned char *)aptr;
  const unsigned char *b = (const unsigned char *)bptr;
  for (size_t i = 0; i < size; i++) {
    if (a[i] < b[i])
      return -1;
    else if (b[i] < a[i])
      return 1;
  }
  return 0;
}

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size) {
  unsigned char *dst = (unsigned char *)dstptr;
  const unsigned char *src = (const unsigned char *)srcptr;
  for (size_t i = 0; i < size; i++)
    dst[i] = src[i];
  return dstptr;
}

void *memset(void *bufptr, int value, size_t size) {
  unsigned char *buf = (unsigned char *)bufptr;
  for (size_t i = 0; i < size; i++)
    buf[i] = (unsigned char)value;
  return bufptr;
}

void *memmove(void *dstptr, const void *srcptr, size_t size) {
  unsigned char *dst = (unsigned char *)dstptr;
  const unsigned char *src = (const unsigned char *)srcptr;
  if (dst < src) {
    for (size_t i = 0; i < size; i++)
      dst[i] = src[i];
  } else {
    for (size_t i = size; i != 0; i--)
      dst[i - 1] = src[i - 1];
  }
  return dstptr;
}

char *strcat(char *destination, const char *source) {
  // make ptr point to the end of destination string
  char *ptr = destination + strlen(destination);

  // Appends characters of source to the destination string
  while (*source != '\0')
    *ptr++ = *source++;

  // null terminate destination string
  *ptr = '\0';

  // destination is returned by standard strcat()
  return destination;
}

int strcmp(const char *a, const char *b) {

  while (*a) {
    if (*a != *b)
      break;

    a++;
    b++;
  }

  return *(const unsigned char *)a - *(const unsigned char *)b;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  unsigned int count = 0;
  while (count < n) {
    if (s1[count] == s2[count]) {
      if (s1[count] == '\0') // quit early because null-termination found
        return 0;
      else
        count++;
    } else
      return s1[count] - s2[count];
  }

  return 0;
}

char *strcpy(char *destination, const char *source) {
  if (destination == NULL) {
    return NULL;
  }

  char *ptr = destination;
  while (*source != '\0') {
    *destination = *source;
    destination++;
    source++;
  }
  *destination = '\0';

  return ptr;
}
char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }
  for (; i < n; i++) {
    dest[i] = '\0';
  }
  return dest;
}

char *strdup(const char *src) {
  size_t size = strlen(src) + 1;
  char *str = kmalloc(size);

  memset(str, 0, size);
  memcpy(str, src, size);
  return str;
}

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}