// Thread with RISC-V RV64I architecture

#pragma once

#include <process/base_thread.hpp>

namespace Hamster
{
    class RV32Thread : public BaseThread
    {
    public:
        RV32Thread(MemorySpace &mem_sp);

        ~RV32Thread() override = default;

        int set_start_addr(uint64_t addr) override;

        int tick() override;

        Syscall get_syscall() override;

        void set_syscall_ret(uint64_t ret) override;
    private:
        int32_t regs[32]; // General purpose registers

        uint32_t pc;   // Program counter

        uint32_t read32(uint32_t addr);
        uint16_t read16(uint32_t addr);
        uint8_t read8(uint32_t addr);

        void write32(uint32_t addr, uint32_t value);
        void write16(uint32_t addr, uint16_t value);
        void write8(uint32_t addr, uint8_t value);

        int execute(uint32_t inst);
    };
} // namespace Hamster

