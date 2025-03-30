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


// ensure it exists
__attribute__((constructor))
static void init_sdcard_swap()
{
    // Initialize the SD card if not already done
    if (!SDCARD_INIT()) {
        assert(!"SD card initialization failed");
        return;
    }
    // create the swap directory
    if (!SD.exists(SDCARD_SWAP_DIR)) {
        SD.mkdir(SDCARD_SWAP_DIR);
    }
}

// swap file name
static char swap_file_name[sizeof(SDCARD_SWAP_DIR) - 1 + 8 + 4 + 1] = // nnnnnnnn.bin + null
    SDCARD_SWAP_DIR; // + null
int make_swap_file_name(int swap_index)
{
    if (swap_index < 0)
    {
        return -1;
    }
    snprintf(swap_file_name + sizeof(SDCARD_SWAP_DIR) - 1, sizeof(swap_file_name) - sizeof(SDCARD_SWAP_DIR), "%08x.bin", swap_index);
    return 0;
}

static uint8_t swap_buffer[HAMSTER_PAGE_SIZE];

int Hamster::_swap_in(int swap_index, uint8_t *data)
{
    if (swap_index < 0)
    {
        return -1;
    }
    // make the file name
    if (make_swap_file_name(swap_index) != 0)
    {
        return -1;
    }
    // open the file
    File f = SD.open(swap_file_name, FILE_READ);
    if (!f)
    {
        return -1;
    }
    // read the data
    int bytes_read = f.read(swap_buffer, sizeof(swap_buffer));
    f.close();

    if (bytes_read != sizeof(swap_buffer))
    {
        return -1;
    }

    // copy the data to the buffer
    memcpy(data, swap_buffer, sizeof(swap_buffer));
    return 0;
}

int Hamster::_swap_out(int swap_index, const uint8_t *data)
{
    if (swap_index < 0)
    {
        return -1;
    }
    // make the file name
    if (make_swap_file_name(swap_index) != 0)
    {
        return -1;
    }
    // open the file
    File f = SD.open(swap_file_name, FILE_WRITE);
    if (!f)
    {
        return -1;
    }
    // write the data
    int bytes_written = f.write(data, sizeof(swap_buffer));
    f.close();

    if (bytes_written != sizeof(swap_buffer))
    {
        return -1;
    }

    return 0;
}

#endif
