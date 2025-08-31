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
            OP_REG = 0b0110011,
            OP_IMM = 0b0010011,
            OP_LOAD = 0b0000011,
            OP_STORE = 0b0100011,
            OP_BRANCH = 0b1100011,
            OP_JAL = 0b1101111,
            OP_JALR = 0b1100111,
            OP_LUI = 0b0110111,
            OP_AUIPC = 0b0010111,
            OP_SYSTEM = 0b1110011,
            OP_MISC_MEM = 0b0001111,

            OP_ATOMIC = 0b0101111,

            OP_FLW = 0b0000111,
            OP_FSW = 0b0100111,
            OP_FMADD = 0b1000011,
            OP_FMSUB = 0b1000111,
            OP_FNMSUB = 0b1001011,
            OP_FNMADD = 0b1001111,
            OP_FREG = 0b1010011,
        };

        uint32_t extract_opcode(uint32_t inst)
        {
            return inst & 0x7F;
        }
    } // namespace

    int RiscVEmulator::read32(uint32_t addr, uint32_t &out)
    {
        if HAMSTER_UNLIKELY (addr & 0b11)
            // unaligned, use slower routine
            return memory->memory.memcpy(&out, addr, sizeof(out));
        return memory->memory.fast_read_aligned(addr, &out, sizeof(out));
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
        remaining_timeslice = HAMSTER_THREAD_TIME_SLICE;
        old_pc = pc;
        return execute();
    }

    RiscVEmulator::ExecuteResult RiscVEmulator::execute()
    {
        uint32_t inst;
        ExecuteResult result;

        if HAMSTER_UNLIKELY (!memory)
        {
            _trace("RiscVEmulator: Memory not initialized!\n");
            result.status = ExecuteResult::Status::Error;
            return result;
        }

        if HAMSTER_UNLIKELY ((PERM_READ | PERM_EXEC) & ~memory->memory.get_permissions(pc))
        {
            _trace("RiscVEmulator: Execute from non-executable address 0x%08x\n", pc);
            result.status = ExecuteResult::Status::IllegalLoad;
            result.illegal_load.address = pc;
            return result;
        }

        if HAMSTER_UNLIKELY (read32(pc, inst) != 0)
        {
            _trace("RiscVEmulator: Failed to read instruction at 0x%08x\n", pc);
            result.status = ExecuteResult::Status::IllegalLoad;
            result.illegal_load.address = pc;
            return result;
        }

        ++total_instructions_executed;

        x[0] = 0;
        switch (extract_opcode(inst))
        {
        case OP_REG:
            result = do_op_reg(inst);
            break;
        case OP_IMM:
            result = do_op_imm(inst);
            break;
        case OP_LOAD:
            result = do_op_load(inst);
            break;
        case OP_STORE:
            result = do_op_store(inst);
            break;
        case OP_BRANCH:
            result = do_op_branch(inst);
            break;
        case OP_JAL:
            result = do_op_jal(inst);
            break;
        case OP_JALR:
            result = do_op_jalr(inst);
            break;
        case OP_LUI:
            result = do_op_lui(inst);
            break;
        case OP_AUIPC:
            result = do_op_auipc(inst);
            break;
        case OP_SYSTEM:
            result = do_op_system(inst);
            break;
        case OP_MISC_MEM:
            result = do_op_misc_mem(inst);
            break;
        case OP_ATOMIC:
            result = do_op_atomic(inst);
            break;
        case OP_FLW:
            result = do_op_flw(inst);
            break;
        case OP_FSW:
            result = do_op_fsw(inst);
            break;
        case OP_FMADD:
            result = do_op_fmadd(inst);
            break;
        case OP_FMSUB:
            result = do_op_fmsub(inst);
            break;
        case OP_FNMADD:
            result = do_op_fnmadd(inst);
            break;
        case OP_FNMSUB:
            result = do_op_fnmsub(inst);
            break;
        case OP_FREG:
            result = do_op_freg(inst);
            break;
        default:
            // Unknown opcode
            _trace("RiscVEmulator: Unknown opcode 0x%02x, instruction: 0x%08x\n", extract_opcode(inst), inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = inst;
            return result;
        }

        return result;
    }
} // namespace Hamster
