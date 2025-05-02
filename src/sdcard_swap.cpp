// SD card swapping

#if defined(ARDUINO) && 1

#include <Arduino.h>
#include <SdFat.h>

#include <platform/platform.hpp>
#include <assert.h>

// Hamster takes ownership of this directory
// and it will create it if it doesn't exist
#define SDCARD_SWAP_DIR "/.swap"

namespace
{
    char name_buffer[sizeof(SDCARD_SWAP_DIR) + 13]; // "/.swap/xxxxxxxx.bin"
    
    int make_swap_name(int index)
    {
        if (index < 0)
            return -1;
        snprintf(name_buffer, sizeof(name_buffer), "%s/%08x.bin", SDCARD_SWAP_DIR, index);
        return 0;
    }
} // namespace

int Hamster::_swap_out(int index, const uint8_t *data)
{
    if (make_swap_name(index) != 0)
        return -1;
    
    SdFile file;
    if (!file.open(name_buffer, O_RDWR | O_CREAT | O_TRUNC))
    {
        return -1;
    }
    if (file.write(data, HAMSTER_PAGE_SIZE) != HAMSTER_PAGE_SIZE)
    {
        file.close();
        return -1;
    }
    file.close();
    return 0;
}

int Hamster::_swap_in(int index, uint8_t *data)
{
    if (make_swap_name(index) != 0)
        return -1;

    SdFile file;
    static uint8_t data_buf[HAMSTER_PAGE_SIZE];
    if (!file.open(name_buffer, O_RDONLY))
    {
        return -1;
    }

    // read into data_buf instead of data
    // to avoid modifying data on error
    if (file.read(data_buf, HAMSTER_PAGE_SIZE) != HAMSTER_PAGE_SIZE)
    {
        file.close();
        return -1;
    }
    file.close();

    memcpy(data, data_buf, HAMSTER_PAGE_SIZE);

    return 0;
}

int Hamster::_swap_rm(int index)
{
    if (make_swap_name(index) != 0)
        return -1;

    SdFile file;
    if (!file.open(name_buffer, O_RDWR))
    {
        return -1;
    }
    if (!file.remove())
    {
        file.close();
        return -1;
    }
    file.close();
    return 0;
}

int Hamster::_swap_rm_all()
{
    SdFile dir;
    SdFile entry;
    if (!dir.open(SDCARD_SWAP_DIR, O_RDONLY))
    {
        return -1;
    }
    if (!dir.isDir())
    {
        dir.close();
        return -2;
    }
    dir.rewindDirectory();
    while (entry.openNext(&dir, O_RDWR))
    {
        entry.remove();
    }
    dir.close();
    return 0;
}

#endif
