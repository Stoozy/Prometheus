#ifndef _STDIO_H
#define _STDIO_H 1

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

#define printf _printf
#define putchar _putchar

int _putchar(char c);
int _printf(const char *, ...);
int puts(const char *);

#ifdef __cplusplus
}
#endif

#endif
