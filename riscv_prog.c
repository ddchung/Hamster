
#include <stdlib.h>

int syscall0(int syscall)
{
    int ret;
    asm volatile(
        "mv a7, %1\n"
        "ecall\n"
        "mv %0, a0"
        : "=r"(ret)
        : "r"(syscall)
        : "a7", "a0"
    );
    return ret;
}

int syscall1(int syscall, int arg1)
{
    int ret;
    asm volatile(
        "mv a7, %1\n"
        "mv a0, %2\n"
        "ecall\n"
        "mv %0, a0"
        : "=r"(ret)
        : "r"(syscall), "r"(arg1)
        : "a7", "a0"
    );
    return ret;
}

void _exit(int status)
{
    syscall1(0, status);
}


int main(void)
{
    int i = syscall0(1);
    return i;
}
