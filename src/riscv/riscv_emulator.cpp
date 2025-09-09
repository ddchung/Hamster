// Hamster risc-v emulator

#include <riscv/riscv_emulator.hpp>
#include <signal.h>
#include <math.h>
#include <cstring>
#include <cfenv>

namespace Hamster
{
    namespace
    {
        enum Opcodes
        {
            // Types:
            // R-type:     funct7 rs2 rs1 funct3 rd     opcode
            // I-type: imm            rs1 funct3 rd     opcode
            // S-type: imm        rs2 rs1 funct3    imm opcode
            // B-type: imm        rs2 rs1 funct3    imm opcode
            // U-type: imm                       rd     opcode
            // J-type: imm                       rd     opcode

            OP_REG      = 0b0110011, //     funct7 rs2 rs1 funct3 rd     opcode
            OP_IMM      = 0b0010011, // imm            rs1 funct3 rd     opcode
            OP_LOAD     = 0b0000011, // imm            rs1 funct3 rd     opcode
            OP_STORE    = 0b0100011, // imm        rs2 rs1 funct3    imm opcode
            OP_BRANCH   = 0b1100011, // imm        rs2 rs1 funct3    imm opcode
            OP_JAL      = 0b1101111, // imm                       rd     opcode
            OP_JALR     = 0b1100111, // imm            rs1 funct3 rd     opcode
            OP_LUI      = 0b0110111,
            OP_AUIPC    = 0b0010111,
            OP_SYSTEM   = 0b1110011,
            OP_MISC_MEM = 0b0001111,
            OP_ATOMIC   = 0b0101111,
            OP_FLW      = 0b0000111,
            OP_FSW      = 0b0100111,
            OP_FMADD    = 0b1000011,
            OP_FMSUB    = 0b1000111,
            OP_FNMSUB   = 0b1001011,
            OP_FNMADD   = 0b1001111,
            OP_FREG     = 0b1010011,
        };
    } // namespace

    int RiscVEmulator::read32(uint32_t addr, uint32_t &out)
    {
        if HAMSTER_UNLIKELY (addr & 0b11)
            // unaligned, use slower routine
            return memory->memory.memcpy(&out, addr, sizeof(out));
        return memory->memory.fast_read_aligned(addr, &out, sizeof(out));
    }

    int RiscVEmulator::fetch(uint32_t addr, uint32_t &out)
    {
        if HAMSTER_UNLIKELY (addr & 0b11)
            return -1; // Execute from unaligned address not allowed
        return memory->memory.fast_fetch_aligned(addr, out);
    }

    int RiscVEmulator::read16(uint32_t addr, uint16_t &out)
    {
        if HAMSTER_UNLIKELY (addr & 0b1)
            return memory->memory.memcpy(&out, addr, sizeof(out));
        return memory->memory.fast_read_aligned(addr, &out, sizeof(out));
    }

    int RiscVEmulator::read8(uint32_t addr, uint8_t &out)
    {
        return memory->memory.fast_read_aligned(addr, &out, sizeof(out));
    }

    int RiscVEmulator::write32(uint32_t addr, uint32_t value)
    {
        if HAMSTER_UNLIKELY (addr & 0b11)
            return memory->memory.memcpy(addr, &value, sizeof(value));
        return memory->memory.fast_write_aligned(addr, &value, sizeof(value));
    }

    int RiscVEmulator::write16(uint32_t addr, uint16_t value)
    {
        if HAMSTER_UNLIKELY (addr & 0b1)
            return memory->memory.memcpy(addr, &value, sizeof(value));

        return memory->memory.fast_write_aligned(addr, &value, sizeof(value));
    }

    int RiscVEmulator::write8(uint32_t addr, uint8_t value)
    {
        return memory->memory.fast_write_aligned(addr, &value, sizeof(value));
    }

    int RiscVEmulator::readf32(uint32_t addr, float &out)
    {
        if HAMSTER_UNLIKELY (addr & 0b11)
            return memory->memory.memcpy(&out, addr, sizeof(out));
        return memory->memory.fast_read_aligned(addr, &out, sizeof(out));
    }

    int RiscVEmulator::readf64(uint32_t addr, double &out)
    {
        if HAMSTER_UNLIKELY (addr & 0b111)
            return memory->memory.memcpy(&out, addr, sizeof(out));
        return memory->memory.fast_read_aligned(addr, &out, sizeof(out));
    }

    int RiscVEmulator::writef32(uint32_t addr, float value)
    {
        if HAMSTER_UNLIKELY (addr & 0b11)
            return memory->memory.memcpy(addr, &value, sizeof(value));
        return memory->memory.fast_write_aligned(addr, &value, sizeof(value));
    }

    int RiscVEmulator::writef64(uint32_t addr, double value)
    {
        if HAMSTER_UNLIKELY (addr & 0b111)
            return memory->memory.memcpy(addr, &value, sizeof(value));
        return memory->memory.fast_write_aligned(addr, &value, sizeof(value));
    }

    RiscVEmulator::ExecuteResult RiscVEmulator::run()
    {
        uint32_t instructions_executed = 0;
        ExecuteResult result;
        result.status = ExecuteResult::Status::Success;
        while (instructions_executed < HAMSTER_THREAD_TIME_SLICE && result.status == ExecuteResult::Status::Success)
            instructions_executed += execute_trace(result);
        return result;
    }
} // namespace Hamster
