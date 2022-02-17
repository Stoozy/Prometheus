__asm__(".section .text\n" 
        ".global _start\n"
        "_start:\n"
        "mov $0, %rbp\n"
        "push %rbp\n"
        "mov %rsp, %rbp\n"
        "call main\n"
        "pop  %rbp\n"
        "ret\n");


//#include <fcntl.h>
// 
//extern void exit(int code);
//extern int main ();
// 
//void _start() {
//    int ex = main();
//    exit(ex);
//}
