#pragma once
#include <stdbool.h>
#include <typedefs.h>

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

void cli();
void sti();
