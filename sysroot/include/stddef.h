#ifndef _SIZE_T
#define _SIZE_T

#ifdef __cplusplus

#define NULL 0L

#else

#define NULL ((void *)0)

#endif

#ifdef __cpluplus
extern "C" {
#endif

typedef long int off_t;
typedef unsigned long size_t;
typedef long ssize_t;

typedef ssize_t ptrdiff_t;

#ifdef __cpluplus
}
#endif

#endif
