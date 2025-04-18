// Teensy 4 implementation

#if (defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40)) && 1

#include <Arduino.h>
#include <SD.h>
#include <platform/platform.hpp>

int Hamster::_init_platform()
{
    Serial.begin(115200);
    while (!Serial)
        ;
    
    // init sd card, if available
    if (SD.begin(BUILTIN_SDCARD))
        Serial.println("SD card found");
    else
        Serial.println("No SD card found");
    return 0;
}

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
