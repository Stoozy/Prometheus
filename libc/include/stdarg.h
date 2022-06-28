#ifndef _STDARG_H
#define _STDARG_H 1

typedef char *va_list;

#define va_start(ap, parmn) (void)((ap) = (char *)(&(parmn) + 1))
#define va_end(ap) (void)((ap) = 0)
#define va_arg(ap, type) (((type *)((ap) = ((ap) + sizeof(type))))[-1])

#define va_copy(d, s) __builtin_va_copy(d, s)

#endif
