
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int syscall0(int);

int main(void)
{
    int i = syscall0(1);
    write(0, "Hello, World!", 13);
    return i;
}
