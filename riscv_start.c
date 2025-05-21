
#include <fcntl.h>
#include <stdarg.h>

extern int main(void);
void _exit(int status);

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

int syscall2(int syscall, int arg1, int arg2)
{
    int ret;
    asm volatile(
        "mv a7, %1\n"
        "mv a0, %2\n"
        "mv a1, %3\n"
        "ecall\n"
        "mv %0, a0"
        : "=r"(ret)
        : "r"(syscall), "r"(arg1), "r"(arg2)
        : "a7", "a0", "a1"
    );
    return ret;
}

int syscall3(int syscall, int arg1, int arg2, int arg3)
{
    int ret;
    asm volatile(
        "mv a7, %1\n"
        "mv a0, %2\n"
        "mv a1, %3\n"
        "mv a2, %4\n"
        "ecall\n"
        "mv %0, a0"
        : "=r"(ret)
        : "r"(syscall), "r"(arg1), "r"(arg2), "r"(arg3)
        : "a7", "a0", "a1", "a2"
    );
    return ret;
}

int syscall4(int syscall, int arg1, int arg2, int arg3, int arg4)
{
    int ret;
    asm volatile(
        "mv a7, %1\n"
        "mv a0, %2\n"
        "mv a1, %3\n"
        "mv a2, %4\n"
        "mv a3, %5\n"
        "ecall\n"
        "mv %0, a0"
        : "=r"(ret)
        : "r"(syscall), "r"(arg1), "r"(arg2), "r"(arg3), "r"(arg4)
        : "a7", "a0", "a1", "a2", "a3"
    );
    return ret;
}

void _exit(int status)
{
    syscall1(0, status);

    while (1);
}

__attribute__((naked))
void _start(void)
{
    _exit(main());
}
