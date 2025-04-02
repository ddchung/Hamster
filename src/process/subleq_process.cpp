// simple subleq interpreter

// structure:
// a b c a b c a b c ...
// a b c = subleq(a, b, c)

#include <process/subleq_process.hpp>
#include <process/process.hpp>
#include <memory/memory_space.hpp>
#include <cassert>

namespace Hamster
{
    namespace
    {
        // state helpers
        // state structure:
        // 0-7: program counter
        // 8-(HAMSTER_PAGE_SIZE - 1): reserved for future use

        uint64_t read_word(MemorySpace& memory_space, uint64_t address)
        {
            auto offset = MemorySpace::get_addr_offset(address);
            assert(offset % 8 == 0);
            assert(offset <= (HAMSTER_PAGE_SIZE - 8));
            return *(uint64_t*)(memory_space.get_page_data(address));
        }

        void write_word(MemorySpace& memory_space, uint64_t address, uint64_t value)
        {
            auto offset = MemorySpace::get_addr_offset(address);
            assert(offset % 8 == 0);
            assert(offset <= (HAMSTER_PAGE_SIZE - 8));
            *(uint64_t*)(memory_space.get_page_data(address)) = value;
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


        // a b c
        uint64_t a = read_word(memory_space, pc);
        uint64_t b = read_word(memory_space, pc + 8);
        uint64_t c = read_word(memory_space, pc + 16);

        // subleq
        b = b - a;
        write_word(memory_space, pc + 8, b);
        
        if (memory_space[b] <= 0)
        {
            // check if c is too close to the end of its page
            if (MemorySpace::get_addr_offset(c) >= (HAMSTER_PAGE_SIZE - 24))
            {
                // TODO: handle page fault
                return -1;
            }
            
            pc = c;
        }
        else
        {
            // next instruction
            pc += 24;
        }

        // write
        write_word(memory_space, PROGRAM_COUNTER_ADDR, pc);

        return 0;
    }
} // namespace Hamster

