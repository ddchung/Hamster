// SD card swapping

#if 1

#include <Arduino.h>
#include <SD.h>

#include <platform/platform.hpp>
#include <assert.h>

// Hamster takes ownership of this directory
// and it will create it if it doesn't exist
#define SDCARD_SWAP_DIR "/.swap"

// platform-dependent SD card initialization
#define SDCARD_INIT() SD.begin(BUILTIN_SDCARD)


static char name_buffer[sizeof(SDCARD_SWAP_DIR) + 13]; // "/.swap/xxxxxxxx.bin"
static uint8_t swap_buffer[HAMSTER_PAGE_SIZE];

int make_swap_name(int index)
{
    if (index < 0)
        return -1;
    if (index > 0xFFFFFFFF)
        return -1;
    snprintf(name_buffer, sizeof(name_buffer), "%s/%08x.bin", SDCARD_SWAP_DIR, index);
    return 0;
}

int Hamster::_swap_out(int index, const uint8_t *data)
{
    if (index < 0)
        return -1;
    if (index > 0xFFFFFFFF)
        return -1;

    if (SDCARD_INIT() != 1)
        return -1;

    if (make_swap_name(index) != 0)
        return -1;

    File f = SD.open(name_buffer, FILE_WRITE);
    if (!f)
        return -1;
    
    if (f.write(data, HAMSTER_PAGE_SIZE) != HAMSTER_PAGE_SIZE)
    {
        f.close();
        return -1;
    }
    f.close();
    return 0;
}

int Hamster::_swap_in(int index, uint8_t *data)
{
    if (index < 0)
        return -1;
    if (index > 0xFFFFFFFF)
        return -1;

    if (SDCARD_INIT() != 1)
        return -1;

    if (make_swap_name(index) != 0)
        return -1;

    File f = SD.open(name_buffer, FILE_READ);
    if (!f)
        return -1;
    
    int len = f.read(swap_buffer, HAMSTER_PAGE_SIZE);
    if (len != HAMSTER_PAGE_SIZE)
    {
        f.close();
        return -1;
    }
    f.close();
    memcpy(data, swap_buffer, HAMSTER_PAGE_SIZE);
    return 0;
}

#endif
