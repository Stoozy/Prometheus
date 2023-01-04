#pragma once
#include <libk/typedefs.h>
#include <stdbool.h>

#define MINORBITS 20
#define MINORMASK ((1U << MINORBITS) - 1)

#define MAJOR(dev) ((unsigned int)((dev) >> MINORBITS))
#define MINOR(dev) ((unsigned int)((dev)&MINORMASK))
#define MKDEV(ma, mi) (((ma) << MINORBITS) | (mi))

#define CONTEXT(n) get_cpu_struct(n)->regs

i32 abs(i32 val);
i64 ll_abs(i64 val);

void swap(char *x, char *y);
char *reverse(char *buf, int i, int j);

char *ulltoa(u64 val, char *buf, u8 base);
char *lltoa(i64 val, char *buf, u8 base);
char *itoa(i32 val, char *buf, u8 base);

void set_bit(u8 *word, u8 bit);
void clear_bit(u8 *word, u8 bit);
bool check_bit(u8 *word, u8 bit);

bool starts_with(const char *a, const char *b);
