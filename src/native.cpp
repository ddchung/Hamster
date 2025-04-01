// Native version

#if !defined(ARDUINO) && 1

#include <platform/platform.hpp>
#include <cstdlib>
#include <cstdio>


void *Hamster::_malloc(size_t size)
{
    return malloc(size);
}

int Hamster::_free(void *ptr)
{
    free(ptr);
    return 0;
}

int Hamster::_log(const char *msg)
{
    return printf("%s", msg);
}

int Hamster::_log(char c)
{
    return printf("%c", c);
}

#endif
