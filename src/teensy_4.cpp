// Teensy 4 implementation

#if (defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40)) && 1

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

int Hamster::_log(const char *msg)
{
    Serial.write(msg);
    return 0;
}

int Hamster::_log(char c)
{
    Serial.write(c);
    return 0;
}

#endif // TEENSY41
