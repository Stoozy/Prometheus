#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void *, const void *, size_t);
void *memcpy(void *__restrict, const void *__restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
char *strcat(char *dest, const char *source);
size_t strlen(const char *);
int strcmp(const char *X, const char *Y);
int strncmp(const char *X, const char *Y, unsigned long n);
char *strcpy(char *destination, const char *source);
char *strncpy(char *dest, const char *src, size_t n);
char *strdup(const char *src);

#ifdef __cplusplus
}
#endif

#endif
