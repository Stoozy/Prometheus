#include <stdint.h>
extern void kprintf();

int fib( int n)
{
    if(n==0 || n == 1)
        return 1;
    return fib(n-1) + fib(n-2);
}

void _start(){
    int i=0;
    while(i<INT32_MAX){
        kprintf("Fibnoacci #%d is %llu \n", i, fib(i));
        ++i;
    } 
}
