// Teensy 4.1 implementation

#include <Arduino.h>
#include <platform/platform.hpp>


void * Hamster::_malloc(size_t size)
{
    return extmem_malloc(size);
}

int Hamster::_free(void *ptr)
{
    extmem_free(ptr);
    return 0;
}
