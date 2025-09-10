// Hamster risc-v emulator

#pragma once

#include <memory/memory_space.hpp>
#include <memory/stl_map.hpp>
#include <cstdint>

namespace Hamster
{
    extern uint64_t total_instructions_executed;

    struct EmulatorMemory
    {
        MemorySpace memory;
        UnorderedMap<uint32_t, int> reserved_mem;
    };

    class RiscVEmulator
    {
        struct DecodedInst
        {
            void *handler;
            uint32_t imm_i, imm_s, imm_b, imm_u, imm_j, inst;
            uint8_t rd, funct3, rs1, rs2, funct7;
        };
    public:
        uint32_t x[32];
        double f[32];
        uint32_t fcsr;
        uint32_t pc;

        int reserved_mem_id;

        EmulatorMemory *memory;

        struct ExecuteResult
        {
            enum class Status : uint8_t
            {
                Success,
                IllegalInstruction,
                IllegalLoad,
                IllegalStore,
                ECALL,
                EBREAK,
                Error,
            };

            Status status;

            union
            {
                struct
                {
                    uint32_t instruction;
                } illegal_instruction;

                struct
                {
                    uint32_t address;
                } illegal_load;

                struct
                {
                    uint32_t address;
                    uint32_t value;
                } illegal_store;
            };
        };

        /**
         * @brief Execute HAMSTER_THREAD_TIME_SLICE instructions, or until an exception occurs
         * @return An ExecuteResult struct containing the result of the execution
         */
        ExecuteResult run();

    private:
        int read8(uint32_t addr, uint8_t &out);
        int read16(uint32_t addr, uint16_t &out);
        int read32(uint32_t addr, uint32_t &out);
        int readf32(uint32_t addr, float &out);
        int readf64(uint32_t addr, double &out);
        int write8(uint32_t addr, uint8_t value);
        int write16(uint32_t addr, uint16_t value);
        int write32(uint32_t addr, uint32_t value);
        int writef32(uint32_t addr, float value);
        int writef64(uint32_t addr, double value);

        uint32_t execute_trace(ExecuteResult &result);
    };
} // namespace Hamster
