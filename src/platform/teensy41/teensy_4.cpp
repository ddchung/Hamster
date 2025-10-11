// Teensy 4 implementation

#if (defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40)) && 1

#include <Arduino.h>
#include <SD.h>
#include <platform/platform.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>
#include <memory/allocator.hpp>
#include <filesystem/device_manager.hpp>
#include <driver/base_tty.hpp>
#include <filesystem/base_romfs.hpp>
#include <driver/block_device_cache.hpp>
#include <driver/base_char_device.hpp>
#include <errno/errno.h>
#include <o1heap.h>

using namespace Hamster;

#define HAMSTER_ROOT_IMG "/rootfs.img"

namespace
{
    class TeensyTTYBackend
    {
    public:
        ssize_t read(void *buf, size_t size)
        {
            // Read from Serial
            size_t bytes_read = 0;
            while (bytes_read < size)
            {
                if (Serial.available())
                    ((uint8_t *)buf)[bytes_read++] = Serial.read();
                else if (Serial8.available())
                    ((uint8_t *)buf)[bytes_read++] = Serial8.read();
                else
                {
                    if (bytes_read == 0)
                    {
                        error = H_EAGAIN;
                        return -1;
                    }
                    break;
                }
                
            }
            return bytes_read;
        }

        ssize_t write(const void *buf, size_t size)
        {
            Serial.write((const char *)buf, size);
            Serial8.write((const char *)buf, size);
            return size;
        }

        int get_win_sz(sys_winsize *ws)
        {
            // Teensy does not support terminal size, return default
            ws->row = 20;   // Default rows
            ws->col = 40;   // Default columns
            ws->xpixel = 0; // Not applicable
            ws->ypixel = 0; // Not applicable
            return 0;       // Success
        }
    };

    class ArduinoRomFsBackend
    {
    public:
        ArduinoRomFsBackend()
        {
            rootfs_file.open(HAMSTER_ROOT_IMG, O_RDONLY);
        }
        ~ArduinoRomFsBackend()
        {
            rootfs_file.close();
        }

        uint64_t get_block_size_log2()
        {
            return 9; // 512 bytes
        }

        int read_block(uint32_t loc, void *buf)
        {
            rootfs_file.seekSet(loc * 512);
            int bytes_read = rootfs_file.read(buf, 512);

            if (bytes_read == 512)
                return 0;
            else if (bytes_read == -1)
            {
                error = H_EIO;
                return -1;
            }
            else
            {
                // Short read
                memset((uint8_t *)buf + bytes_read, 0, 512 - bytes_read);
                return 0;
            }
        }

        int write_block(uint32_t loc, const void *buf)
        {
            // Read-only
            error = H_EROFS;
            return -1;
        }

        bool is_read_only()
        {
            return true;
        }

    private:
        SdFile rootfs_file;
    };

    // make speaker buffer volatile because it is accessed from interrupt
    volatile class
    {
    public:
        void push(uint8_t c) volatile
        {
            assert(count < sizeof(buffer)); // ensure we don't overflow
            buffer[head] = c;
            head = (head + 1) % sizeof(buffer);
            count++;
        }

        uint8_t pop() volatile
        {
            assert(count > 0); // ensure we don't underflow
            uint8_t c = buffer[tail];
            tail = (tail + 1) % sizeof(buffer);
            count--;
            return c;
        }

        uint32_t size() const volatile
        {
            return count;
        }

        constexpr static uint32_t max_size()
        {
            return 65536;
        }

    private:
        uint8_t buffer[65536];
        uint32_t count = 0;
        uint16_t head = 0;
        uint16_t tail = 0;
    } speaker_buffer;

    struct TeensyLEDDevice
    {
    };
    struct TeensySpeakerDevice
    {
    };
    struct TeensySerialDevice
    {
    };

    IntervalTimer speaker_timer;

    void speaker_write_sample(uint8_t sample)
    {
        // Map sample bits to GPIO6 bits
        const uint32_t pin_bits[8] = {
            18, // pin 14
            19, // pin 15
            23, // pin 16
            22, // pin 17
            17, // pin 18
            16, // pin 19
            26, // pin 20
            27  // pin 21
        };

        // Build a mask for all 8 pins at once
        uint32_t new_val = 0;
        for (int i = 0; i < 8; i++)
            if (sample & (1 << i))
                new_val |= (1UL << pin_bits[i]);

        // Clear and set all 8 pins in one operation
        uint32_t all_mask = (1UL << 16) | (1UL << 17) | (1UL << 18) | (1UL << 19) |
                            (1UL << 22) | (1UL << 23) | (1UL << 26) | (1UL << 27);

        GPIO6_DR = (GPIO6_DR & ~all_mask) | new_val;
    }

    void speaker_timer_isr()
    {
        if (speaker_buffer.size() > 0)
        {
            uint8_t sample = speaker_buffer.pop();
            speaker_write_sample(sample);
        }
        else
        {
            speaker_timer.end();

            // No data, output mid-level (128)
            speaker_write_sample(128);
        }
    }

    using ArduinoRomFs = BaseRomFs<BlockDeviceCache<ArduinoRomFsBackend>>;

    O1HeapInstance *ext_heap;
    O1HeapInstance *ram2_heap;
    EXTMEM uint8_t extmem_buffer[16 * 1024 * 1024];
    DMAMEM uint8_t ram2_buffer[450 * 1024];
} // namespace

namespace Hamster
{
    template <>
    ssize_t CharacterDeviceImpl<TeensyLEDDevice>::write(const void *buffer, size_t size)
    {
        uint8_t *data = (uint8_t *)buffer;
        for (size_t i = 0; i < size; i++)
        {
            switch (data[i])
            {
            case '1':
            case 0x1:
                digitalWrite(LED_BUILTIN, HIGH);
                break;
            case '0':
            case 0x0:
                digitalWrite(LED_BUILTIN, LOW);
                break;
            }
        }
        return size;
    }

    template <>
    ssize_t CharacterDeviceImpl<TeensySpeakerDevice>::write(const void *buffer, size_t size)
    {
        uint32_t remaining = speaker_buffer.max_size() - speaker_buffer.size();
        if (size > remaining)
            size = remaining;
        if (remaining == 0)
        {
            // Buffer full, block until some space is available
            error = H_EAGAIN;
            return -1;
        }
        
        // Start timer if just starting up
        if (speaker_buffer.size() == 0)
            speaker_timer.begin(speaker_timer_isr, 21);
        
        noInterrupts();
        const uint8_t *data = (const uint8_t *)buffer;
        for (size_t i = 0; i < size; i++)
            speaker_buffer.push(data[i]);
        interrupts();
        return size;
    }

    template <>
    ssize_t CharacterDeviceImpl<TeensySpeakerDevice>::poll(int op)
    {
        if (op & 0x2) // write
        {
            if (speaker_buffer.size() >= speaker_buffer.max_size())
                return 0; // not ready
        }
        return 1; // ready
    }

    template <>
    ssize_t CharacterDeviceImpl<TeensySerialDevice>::read(void *buf, size_t size)
    {
        size_t avail = SerialUSB1.available();
        size = min(avail, size);
        if (size == 0)
        {
            Hamster::error = EAGAIN;
            return -1;
        }
        SerialUSB1.readBytes((char *)buf, size);
        return size;
    }

    template <>
    ssize_t CharacterDeviceImpl<TeensySerialDevice>::write(const void *buf, size_t size)
    {
        size_t avail = SerialUSB1.availableForWrite();
        size = min(avail, size);
        if (size == 0)
        {
            Hamster::error = EAGAIN;
            return -1;
        }
        SerialUSB1.write((const char *)buf, size);
        return size;
    }

    template <>
    ssize_t CharacterDeviceImpl<TeensySerialDevice>::poll(int op)
    {
        if (op & 0x1)
        {
            // read
            if (SerialUSB1.available() == 0)
                return 0;
        }
        if (op & 0x2)
        {
            // write
            if (SerialUSB1.availableForWrite() == 0)
                return 0;
        }

        return 1;
    }
} // namespace

void Hamster::_init_allocator()
{
    ext_heap = o1heapInit(extmem_buffer, sizeof(extmem_buffer));
    ram2_heap = o1heapInit(ram2_buffer, sizeof(ram2_buffer));
}

int Hamster::_init_platform()
{
    Serial8.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(14, OUTPUT);
    pinMode(15, OUTPUT);
    pinMode(16, OUTPUT);
    pinMode(17, OUTPUT);
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);
    pinMode(20, OUTPUT);
    pinMode(21, OUTPUT);

    // init sd card, if available
    if (SD.begin(254))
        Serial.println("SD card found");
    else
    {
        // Set color to red
        Serial.print("\e[31m");
        Serial.println("! NO SD CARD FOUND !");
        SD.sdfs.initErrorPrint(&Serial);
        Serial.println("aborting...");
        Serial.print("\e[0m");
        abort();
    }
    return 0;
}

int Hamster::_mount_rootfs()
{
    vfs.mount("/", alloc<ArduinoRomFs>());
    Hamster::vfs.mkdir("/dev", 0755);
    Hamster::vfs.mkdir("/tmp", 0755);
    BaseFilesystem *ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/dev", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);
    ramfs = Hamster::alloc<Hamster::RamFs>();
    Hamster::vfs.mount("/tmp", ramfs) == 0 ? (void)0 : Hamster::dealloc(ramfs);

    auto console_device = Hamster::alloc<BaseTTYDriver<TeensyTTYBackend>>();
    Hamster::device_manager.register_device({5, 1}, console_device);
    Hamster::vfs.mknod("/dev/console", {5, 1}, 0666);
    Hamster::vfs.symlink("/dev/tty", "/dev/console");

    auto led_device = Hamster::alloc<CharacterDevice<TeensyLEDDevice>>();
    Hamster::device_manager.register_device({0, 1}, led_device);
    Hamster::vfs.mknod("/dev/led", {0, 1}, 0666);

    auto speaker_device = Hamster::alloc<CharacterDevice<TeensySpeakerDevice>>();
    Hamster::device_manager.register_device({0, 2}, speaker_device);
    Hamster::vfs.mknod("/dev/speaker", {0, 2}, 0666);

    auto serial_device = Hamster::alloc<CharacterDevice<TeensySerialDevice>>();
    Hamster::device_manager.register_device({0, 3}, serial_device);
    Hamster::vfs.mknod("/dev/serial", {0, 3}, 0666);

    return 0;
}

void *Hamster::_malloc(size_t size)
{
    void *ptr = nullptr;
    if (!ptr && size < 1024)
        ptr = o1heapAllocate(ram2_heap, size);
    if (!ptr)
        ptr = o1heapAllocate(ext_heap, size);
    return ptr;
}

int Hamster::_free(void *ptr)
{
    if (ptr >= extmem_buffer &&
        ptr < extmem_buffer + sizeof(extmem_buffer))
        o1heapFree(ext_heap, ptr);

    else if (ptr >= ram2_buffer &&
             ptr < ram2_buffer + sizeof(ram2_buffer))
        o1heapFree(ram2_heap, ptr);
    else
        __builtin_unreachable();
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

uint64_t Hamster::_get_sys_time()
{
    return millis();
}

size_t Hamster::_get_free_memory()
{
    void *ptr = nullptr;
    for (size_t i = 16 * 1024 * 1024; i; i -= 512 * 1024)
    {
        ptr = _malloc(i);
        if (ptr)
        {
            _free(ptr);
            return i - 1;
        }
    }
    return 0;
}

#ifndef NTRACE
void Hamster::_trace(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SerialUSB2.vprintf(fmt, args);
    va_end(args);
}
#endif

#endif // TEENSY41
