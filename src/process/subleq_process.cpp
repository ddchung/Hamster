// simple subleq interpreter

// structure:
// a b c a b c a b c ...
// a b c = subleq(a, b, c)

#include <process/subleq_process.hpp>
#include <process/process.hpp>
#include <memory/memory_space.hpp>
#include <cassert>

#include <cstdio>

namespace Hamster
{
    namespace
    {
        // state helpers
        // state structure:
        // 0-7: program counter
        // 8-(HAMSTER_PAGE_SIZE - 1): reserved for future use
        
        uint8_t data_buffer[8] = {0};

        uint64_t read_word(MemorySpace& memory_space, uint64_t address)
        {
            for (int i = 0; i < 8; ++i)
            {
                data_buffer[i] = memory_space[address + i];
            }
            return *(uint64_t*)data_buffer;
        }

        void write_word(MemorySpace& memory_space, uint64_t address, uint64_t value)
        {
            *(uint64_t*)data_buffer = value;
            for (int i = 0; i < 8; ++i)
            {
                memory_space[address + i] = data_buffer[i];
            }
        }

        constexpr uint64_t PROGRAM_COUNTER_ADDR = 0;
    }

    int ProcessTypes::subleq(Process& process)
    {
        auto& memory_space = ProcessHelper::get_memory_space(process);

        // allocate first page for state-keeping
        (void)memory_space.allocate_page(0);

        // read program counter
        uint64_t pc = read_word(memory_space, PROGRAM_COUNTER_ADDR);

        uint64_t a_adr = pc;
        uint64_t b_adr = pc + 8;
        uint64_t c_adr = pc + 16;

        uint64_t a = read_word(memory_space, a_adr);
        uint64_t b = read_word(memory_space, b_adr);
        uint64_t c = read_word(memory_space, c_adr);

        if (c == pc)
        {
            // syscall stub
            if (a == 0)
            {
                return 1;
            }
            if (a == 1)
            {
                printf("Hello, World!\n");
            }
            if (a == 2)
            {
                printf("%llu", b);
            }
        }

        write_word(memory_space, b, read_word(memory_space, b) - read_word(memory_space, a));

        if (read_word(memory_space, b) <= 0)
        {
            if (c % 8 != 0)
            {
                // TODO: handle alignment fault
                return -1;
            }
            write_word(memory_space, PROGRAM_COUNTER_ADDR, c);
        }
        else
        {
            write_word(memory_space, PROGRAM_COUNTER_ADDR, pc + 24);
        }
        return 0;
    }
} // namespace Hamster

