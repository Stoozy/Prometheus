#include "typedefs.h"
#include <stdbool.h>

int32_t abs(int32_t val);
int64_t ll_abs(int64_t val);

void swap(char *x, char *y); 
char* reverse(char *buffer, int i, int j); 
char* itoa(int32_t value, char* buffer, int base);
char* lltoa(int64_t value, char* buffer, int base);
char* ulltoa(uint64_t value, char* buffer, int base);

void _set_bit(uint8_t * word, uint8_t bit);
void _clear_bit(uint8_t * word, uint8_t bit);
bool _check_bit(uint8_t * word, uint8_t bit);
