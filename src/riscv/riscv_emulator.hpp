// Hamster risc-v emulator

#pragma once

#include <memory/memory_space.hpp>
#include <memory/stl_map.hpp>
#include <cstdint>

namespace Hamster
{
    struct EmulatorMemory
    {
        MemorySpace memory;
        UnorderedMap<uint32_t, int> reserved_mem;
    };

    class RiscVEmulator
    {
    public:
        uint32_t x[32];
        double f[32];
        uint32_t fcsr;
        uint32_t pc;
        uint32_t old_pc;

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
        uint8_t remaining_timeslice;
    
        ExecuteResult execute();

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

        ExecuteResult do_op_reg(uint32_t inst);
        ExecuteResult do_op_imm(uint32_t inst);
        ExecuteResult do_op_load(uint32_t inst);
        ExecuteResult do_op_store(uint32_t inst);
        ExecuteResult do_op_branch(uint32_t inst);
        ExecuteResult do_op_jal(uint32_t inst);
        ExecuteResult do_op_jalr(uint32_t inst);
        ExecuteResult do_op_lui(uint32_t inst);
        ExecuteResult do_op_auipc(uint32_t inst);
        ExecuteResult do_op_system(uint32_t inst);
        ExecuteResult do_op_misc_mem(uint32_t inst);
        ExecuteResult do_op_atomic(uint32_t inst);
        ExecuteResult do_op_flw(uint32_t inst);
        ExecuteResult do_op_fsw(uint32_t inst);
        ExecuteResult do_op_fmadd(uint32_t inst);
        ExecuteResult do_op_fmsub(uint32_t inst);
        ExecuteResult do_op_fnmsub(uint32_t inst);
        ExecuteResult do_op_fnmadd(uint32_t inst);
        ExecuteResult do_op_freg(uint32_t inst);

        ExecuteResult do_invalid_op(uint32_t inst);

        static inline constexpr ExecuteResult (RiscVEmulator::*opcode_jumptable[])(uint32_t) = 
        {
            &RiscVEmulator::do_op_load,
            &RiscVEmulator::do_op_flw,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_op_misc_mem,
            &RiscVEmulator::do_op_imm,
            &RiscVEmulator::do_op_auipc,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_op_store,
            &RiscVEmulator::do_op_fsw,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_op_atomic,
            &RiscVEmulator::do_op_reg,
            &RiscVEmulator::do_op_lui,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_op_fmadd,
            &RiscVEmulator::do_op_fmsub,
            &RiscVEmulator::do_op_fnmsub,
            &RiscVEmulator::do_op_fnmadd,
            &RiscVEmulator::do_op_freg,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_op_branch,
            &RiscVEmulator::do_op_jalr,
            &RiscVEmulator::do_invalid_op,
            &RiscVEmulator::do_op_jal,
            &RiscVEmulator::do_op_system
        };
        static inline constexpr size_t opc_jumptab_sz = sizeof(opcode_jumptable);
    };
} // namespace Hamster
