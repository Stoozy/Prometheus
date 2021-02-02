#include  <kernel/util.h>
#include  <kernel/typedefs.h>

int32_t abs(int32_t val){
    if(val<0) return -val;
    else return val;
}


int64_t ll_abs(int64_t val){
    if(val<0) return -val;
    else return val;
}

void swap(char *x, char *y) {
    char t = *x; *x = *y; *y = t;
}
 
// function to reverse buffer[i..j]
char* reverse(char *buffer, int i, int j)
{
    while (i < j)
        swap(&buffer[i++], &buffer[j--]);
 
    return buffer;
}
char* ulltoa(uint64_t val, char* buffer, int base){
    // invalid input
    if (base < 2 || base > 32)
        return buffer;
 
    uint64_t n = val;
    uint64_t i = 0;

    while (n){
        int8_t r = n % base; 
        if (r >= 10) 
            buffer[i++] = 65 + (r - 10);
        else
            buffer[i++] = 48 + r; 
        n = n / base;
    }
 
    // if number is 0
    if (i == 0)
        buffer[i++] = '0';
 
    buffer[i] = '\0'; // null terminate string
 
    // reverse the string and return it
    return reverse(buffer, 0, i - 1);

}

char* lltoa(int64_t value, char* buffer, int base)
{
    // invalid input
    if (base < 2 || base > 32)
        return buffer;
 
    int64_t n = ll_abs(value);
     
    unsigned long i = 0;
    while (n)
    {
        int8_t r = n % base; 
        if (r >= 10) 
            buffer[i++] = 65 + (r - 10);
        else
            buffer[i++] = 48 + r; 
        n = n / base;
    }
 
    // if number is 0
    if (i == 0)
        buffer[i++] = '0';
 
    // If base is 10 and value is negative, the resulting string 
    // is preceded with a minus sign (-)
    // With any other base, value is always considered unsigned
    if (value < 0 && base == 10)
        buffer[i++] = '-';
 
    buffer[i] = '\0'; // null terminate string
 
    // reverse the string and return it
    return reverse(buffer, 0, i - 1);
}

// Iterative function to implement itoa() function in C
char* itoa(int32_t value, char* buffer, int base)
{
    // invalid input
    if (base < 2 || base > 32)
        return buffer;
 
    // consider absolute value of number
    int32_t n = abs(value);
 
    int32_t i = 0;
    while (n)
    {
        int r = n % base; 
        if (r >= 10) 
            buffer[i++] = 65 + (r - 10);
        else
            buffer[i++] = 48 + r; 
        n = n / base;
    }
 
    // if number is 0
    if (i == 0)
        buffer[i++] = '0';
 
    // If base is 10 and value is negative, the resulting string 
    // is preceded with a minus sign (-)
    // With any other base, value is always considered unsigned
    if (value < 0 && base == 10)
        buffer[i++] = '-';
 
    buffer[i] = '\0'; // null terminate string
 
    // reverse the string and return it
    return reverse(buffer, 0, i - 1);
}
 
void _set_bit(uint8_t *word, uint8_t bit ){
    *word |= (1 << bit);
    return;
}
void _clear_bit(uint8_t *word, uint8_t  bit){
    *word &= ~(1 << bit);
    return;
}


bool _check_bit(uint8_t * word, uint8_t bit){
    return *word & (1 << bit);
}


