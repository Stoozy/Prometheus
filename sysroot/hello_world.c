int main(){
    char msg[14] = "Hello World\n";
    asm ("mov $4, %%rsi\n\t\
            mov $1, %%r8 \n\t\
            mov %0, %%r9\n\t\
            mov $14, %%r10\n\t\
            syscall" : :"r"(&msg) : );
    for(;;);
}
