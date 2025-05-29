// RISC-V RV32G thread

#include <process/thread.hpp>
#include <process/process.hpp>
#include <memory/stl_map.hpp>
#include <syscall/syscall.hpp>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <utility>
#include <signal.h>
#include <math.h>
#include <cfenv>

// for debugging
#include <cstdio>

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

            OP_CSR = 0b1110011,
        };

        enum Funct3
        {
            FUNCT3_BEQ = 0b000,
            FUNCT3_BNE = 0b001,
            FUNCT3_BLT = 0b100,
            FUNCT3_BGE = 0b101,
            FUNCT3_BLTU = 0b110,
            FUNCT3_BGEU = 0b111,

            FUNCT3_LB = 0b000,
            FUNCT3_LH = 0b001,
            FUNCT3_LW = 0b010,
            FUNCT3_LBU = 0b100,
            FUNCT3_LHU = 0b101,

            FUNCT3_SB = 0b000,
            FUNCT3_SH = 0b001,
            FUNCT3_SW = 0b010,

            FUNCT3_ADD_SUB = 0b000,
            FUNCT3_SLL = 0b001,
            FUNCT3_SLT = 0b010,
            FUNCT3_SLTU = 0b011,
            FUNCT3_XOR = 0b100,
            FUNCT3_SRL_SRA = 0b101,
            FUNCT3_OR = 0b110,
            FUNCT3_AND = 0b111,

            FUNCT3_FENCE = 0b000,
            FUNCT3_FENCE_I = 0b001,

            FUNCT3_MUL = 0b000,
            FUNCT3_MULH = 0b001,
            FUNCT3_MULHSU = 0b010,
            FUNCT3_MULHU = 0b011,
            FUNCT3_DIV = 0b100,
            FUNCT3_DIVU = 0b101,
            FUNCT3_REM = 0b110,
            FUNCT3_REMU = 0b111,

            FUNCT3_ECALL_EBREAK = 0b000,

            FUNCT3_CSRRW = 0b001,
            FUNCT3_CSRRS = 0b010,
            FUNCT3_CSRRC = 0b011,
            FUNCT3_CSRRWI = 0b101,
            FUNCT3_CSRRSI = 0b110,
            FUNCT3_CSRRCI = 0b111,
        };

        enum Funct5
        {
            FUNCT5_LR = 0b00010,
            FUNCT5_SC = 0b00011,
            FUNCT5_AMOSWAP = 0b00001,
            FUNCT5_AMOADD = 0b00000,
            FUNCT5_AMOAND = 0b01100,
            FUNCT5_AMOOR = 0b01000,
            FUNCT5_AMOXOR = 0b00100,
            FUNCT5_AMOMAX = 0b10100,
            FUNCT5_AMOMIN = 0b10000,
            FUNCT5_AMOMAXU = 0b11100,
            FUNCT5_AMOMINU = 0b11000,
        };

        enum RoundingMode
        {
            ROUND_RNE = 0b000,
            ROUND_RTZ = 0b001,
            ROUND_RDN = 0b010,
            ROUND_RUP = 0b011,
            ROUND_RMM = 0b100,
            ROUND_DYN = 0b111,
        };

        uint32_t sign_extend(uint32_t value, uint32_t bits)
        {
            if (value & (1 << (bits - 1)))
            {
                value |= ~((1 << bits) - 1);
            }
            return value;
        }

        uint32_t extract_opcode(uint32_t inst)
        {
            return inst & 0x7F;
        }

        uint32_t extract_rd(uint32_t inst)
        {
            return (inst >> 7) & 0x1F;
        }

        uint32_t extract_funct3(uint32_t inst)
        {
            return (inst >> 12) & 0x07;
        }

        uint32_t extract_rs1(uint32_t inst)
        {
            return (inst >> 15) & 0x1F;
        }

        uint32_t extract_rs2(uint32_t inst)
        {
            return (inst >> 20) & 0x1F;
        }

        uint32_t extract_rs3(uint32_t inst)
        {
            return (inst >> 27) & 0x1F;
        }

        uint32_t extract_funct7(uint32_t inst)
        {
            return (inst >> 25) & 0x7F;
        }

        uint32_t extract_funct5(uint32_t inst)
        {
            return (inst >> 27) & 0x1F;
        }

        uint32_t extract_imm_i(uint32_t inst)
        {
            return sign_extend((inst >> 20) & 0xFFF, 12);
        }

        uint32_t extract_imm_s(uint32_t inst)
        {
            return sign_extend(((inst >> 25) & 0x7F) << 5 | ((inst >> 7) & 0x1F), 12);
        }

        uint32_t extract_imm_b(uint32_t inst)
        {
            return sign_extend(
                ((inst >> 31) & 0x1) << 12 |     // imm[12]
                    ((inst >> 7) & 0x1) << 11 |  // imm[11]
                    ((inst >> 25) & 0x3F) << 5 | // imm[10:5]
                    ((inst >> 8) & 0xF) << 1,    // imm[4:1]
                13);
        }

        uint32_t extract_imm_u(uint32_t inst)
        {
            return (inst & 0xFFFFF000);
        }

        uint32_t extract_imm_j(uint32_t inst)
        {
            return sign_extend(
                ((inst >> 31) & 0x1) << 20 |      // imm[20]
                    ((inst >> 12) & 0xFF) << 12 | // imm[19:12]
                    ((inst >> 20) & 0x1) << 11 |  // imm[11]
                    ((inst >> 21) & 0x3FF) << 1,  // imm[10:1]
                21);
        }

        bool is_double_precision(uint32_t inst)
        {
            return (extract_funct7(inst) & 0b11) == 0b01;
        }

        void write_float_to_double(float f, double &d)
        {
            static char buf_double[8]{0};

            memcpy(buf_double, &f, sizeof(float));
            memcpy(&d, buf_double, sizeof(double));
        }

        float read_float_from_double(double d)
        {
            static char buf_float[8]{0};

            memcpy(buf_float, &d, sizeof(double));
            float f;
            memcpy(&f, buf_float, sizeof(float));
            return f;
        }

        void set_round_mode(uint8_t round_mode, uint32_t fcsr)
        {
            switch (round_mode)
            {
            case ROUND_RNE:
                std::fesetround(FE_TONEAREST);
                break;
            case ROUND_RTZ:
                std::fesetround(FE_TOWARDZERO);
                break;
            case ROUND_RDN:
                std::fesetround(FE_DOWNWARD);
                break;
            case ROUND_RUP:
                std::fesetround(FE_UPWARD);
                break;
            case ROUND_RMM:
                std::fesetround(FE_TOWARDZERO);
                break;
            case ROUND_DYN:
                // Extract rounding mode from FCSR
                round_mode = (fcsr >> 5) & 0b111;
                if (round_mode == ROUND_DYN)
                    return;
                set_round_mode(round_mode, fcsr);
                break;
            }
        }

        uint32_t classify_float(float f)
        {
            if (isnan(f))
            {
                // Check signaling vs quiet NaN
                union
                {
                    float f;
                    uint32_t u;
                } u = {f};
                return (u.u & 0x00400000) ? (1u << 9) : (1u << 8); // bit 22 = quiet NaN flag
            }
            if (isinf(f))
            {
                return signbit(f) ? (1u << 0) : (1u << 7);
            }
            if (f == 0.0f)
            {
                return signbit(f) ? (1u << 3) : (1u << 4);
            }
            if (fpclassify(f) == FP_SUBNORMAL)
            {
                return signbit(f) ? (1u << 2) : (1u << 5);
            }
            // Normal number
            return signbit(f) ? (1u << 1) : (1u << 6);
        }

        uint32_t classify_double(double d)
        {
            if (isnan(d))
            {
                union
                {
                    double d;
                    uint64_t u;
                } u = {d};
                return (u.u & 0x0008000000000000ULL) ? (1u << 9) : (1u << 8); // bit 51 = quiet NaN flag
            }
            if (isinf(d))
            {
                return signbit(d) ? (1u << 0) : (1u << 7);
            }
            if (d == 0.0)
            {
                return signbit(d) ? (1u << 3) : (1u << 4);
            }
            if (fpclassify(d) == FP_SUBNORMAL)
            {
                return signbit(d) ? (1u << 2) : (1u << 5);
            }
            return signbit(d) ? (1u << 1) : (1u << 6);
        }
    } // namespace

    Thread::Thread(Process *process, size_t id)
        : state(ThreadState::RUNNING), process(process), id(id),
          x{0}, f{0.0}, fcsr(0), pc(0),
          pending_signal(0), signal_mask(0)
    {
        // Set stack pointer to top of memory
        x[2] = 0xFFFFFFFF;
    }

    Thread::Thread(Thread &&other)
        : state(other.state), process(other.process),
          pause_callbacks(std::move(other.pause_callbacks)),
          id(other.id),
          x{0}, f{0.0}, fcsr(other.fcsr), pc(other.pc),
          pending_signal(other.pending_signal), signal_mask(other.signal_mask)
    {
        std::memcpy(x, other.x, sizeof(x));
        std::memcpy(f, other.f, sizeof(f));
        other.process = nullptr;
        other.state = ThreadState::ENDED;
        other.id = 0;
    }

    Thread &Thread::operator=(Thread &&other)
    {
        if (this == &other)
            return *this;
        std::swap(state, other.state);
        std::swap(process, other.process);
        std::swap(id, other.id);
        std::swap(pause_callbacks, other.pause_callbacks);
        std::memcpy(x, other.x, sizeof(x));
        std::memcpy(f, other.f, sizeof(f));
        std::swap(fcsr, other.fcsr);
        std::swap(pc, other.pc);
        std::swap(pending_signal, other.pending_signal);
        std::swap(signal_mask, other.signal_mask);
        return *this;
    }

    void Thread::tick()
    {
        assert(state == ThreadState::RUNNING);

        // Fetch the instruction
        uint32_t inst;
        if (read32(pc, inst) != 0)
            return;

        if (pending_signal & signal_mask)
            handle_signal();
        
        ++tick_count;
        
        pc += 4;

        int ret = execute(inst);

        if (ret < 0)
            return;

        // done
    }

    void Thread::pause(const std::function<void(Thread &)> &callback)
    {
        pause_callbacks.push_back(callback);
    }

    void Thread::resume()
    {
        if (pause_callbacks.empty())
            return;
        pause_callbacks.pop_back();
    }

    const std::function<void(Thread &)> &Thread::get_current_pause_callback() const
    {
        static std::function<void(Thread &)> null_callback;
        if (pause_callbacks.empty())
            return null_callback;
        return pause_callbacks.back();
    }

    void Thread::signal(int signal)
    {
        pending_signal |= signal;
    }

    int Thread::read32(uint32_t addr, uint32_t &out)
    {
        if (!process->memory_space.is_allocated(addr) ||
            !process->memory_space.is_allocated(addr + sizeof(out) - 1))
        {
            // Read from unallocated memory
            signal(SIGSEGV);
            return -1;
        }
        return process->memory_space.memcpy(&out, addr, sizeof(out));
    }

    int Thread::read16(uint32_t addr, uint16_t &out)
    {
        if (!process->memory_space.is_allocated(addr) ||
            !process->memory_space.is_allocated(addr + sizeof(out) - 1))
        {
            // Read from unallocated memory
            signal(SIGSEGV);
            return -1;
        }
        return process->memory_space.memcpy(&out, addr, sizeof(out));
    }

    int Thread::read8(uint32_t addr, uint8_t &out)
    {
        if (!process->memory_space.is_allocated(addr) ||
            !process->memory_space.is_allocated(addr + sizeof(out) - 1))
        {
            // Read from unallocated memory
            signal(SIGSEGV);
            return -1;
        }
        return process->memory_space.memcpy(&out, addr, sizeof(out));
    }

    int Thread::write32(uint32_t addr, uint32_t value)
    {
        // Note that writing to unallocating memory will allocate it
        return process->memory_space.memcpy(addr, &value, sizeof(value));
    }

    int Thread::write16(uint32_t addr, uint16_t value)
    {
        return process->memory_space.memcpy(addr, &value, sizeof(value));
    }

    int Thread::write8(uint32_t addr, uint8_t value)
    {
        return process->memory_space.memcpy(addr, &value, sizeof(value));
    }

    int Thread::readf32(uint32_t addr, float &out)
    {
        if (!process->memory_space.is_allocated(addr) ||
            !process->memory_space.is_allocated(addr + sizeof(out) - 1))
        {
            // Read from unallocated memory
            signal(SIGSEGV);
            return -1;
        }
        return process->memory_space.memcpy(&out, addr, sizeof(out));
    }

    int Thread::readf64(uint32_t addr, double &out)
    {
        if (!process->memory_space.is_allocated(addr) ||
            !process->memory_space.is_allocated(addr + sizeof(out) - 1))
        {
            // Read from unallocated memory
            signal(SIGSEGV);
            return -1;
        }
        return process->memory_space.memcpy(&out, addr, sizeof(out));
    }

    int Thread::writef32(uint32_t addr, float value)
    {
        // Note that writing to unallocating memory will allocate it
        return process->memory_space.memcpy(addr, &value, sizeof(value));
    }

    int Thread::writef64(uint32_t addr, double value)
    {
        // Note that writing to unallocating memory will allocate it
        return process->memory_space.memcpy(addr, &value, sizeof(value));
    }

    int Thread::execute(uint32_t inst)
    {
        printf("Executing instruction %08x at PC %08x\n", inst, pc - 4);
        x[0] = 0;
        switch (extract_opcode(inst))
        {
        case OP_REG:
        {
            if ((extract_funct7(inst) & 0x1) == 0)
                switch (extract_funct3(inst))
                {
                    // Base integer instructions
                case FUNCT3_ADD_SUB:
                    if ((extract_funct7(inst) & 0x20) == 0)
                        // ADD
                        x[extract_rd(inst)] =
                            x[extract_rs1(inst)] + x[extract_rs2(inst)];
                    else
                        // SUB
                        x[extract_rd(inst)] =
                            x[extract_rs1(inst)] - x[extract_rs2(inst)];
                    break;
                case FUNCT3_XOR:
                    x[extract_rd(inst)] =
                        x[extract_rs1(inst)] ^ x[extract_rs2(inst)];
                    break;
                case FUNCT3_OR:
                    x[extract_rd(inst)] =
                        x[extract_rs1(inst)] | x[extract_rs2(inst)];
                    break;
                case FUNCT3_AND:
                    x[extract_rd(inst)] =
                        x[extract_rs1(inst)] & x[extract_rs2(inst)];
                    break;
                case FUNCT3_SLL:
                    x[extract_rd(inst)] =
                        x[extract_rs1(inst)] << (x[extract_rs2(inst)] & 0x1F);
                    break;
                case FUNCT3_SRL_SRA:
                    if ((extract_funct7(inst) & 0x20) == 0)
                        // SRL
                        x[extract_rd(inst)] =
                            x[extract_rs1(inst)] >> (x[extract_rs2(inst)] & 0x1F);
                    else
                    {
                        // SRA
                        x[extract_rd(inst)] = (int32_t)x[extract_rs1(inst)] >> (x[extract_rs2(inst)] & 0x1F);
                    }
                    break;
                case FUNCT3_SLT:
                    x[extract_rd(inst)] =
                        (int32_t)x[extract_rs1(inst)] < (int32_t)x[extract_rs2(inst)];
                    break;
                case FUNCT3_SLTU:
                    x[extract_rd(inst)] =
                        x[extract_rs1(inst)] < x[extract_rs2(inst)];
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
            else
                switch (extract_funct3(inst))
                {
                    // -M extension instructions
                case FUNCT3_MUL:
                    x[extract_rd(inst)] =
                        (int64_t)x[extract_rs1(inst)] * (int64_t)x[extract_rs2(inst)];
                    break;
                case FUNCT3_MULH:
                    x[extract_rd(inst)] =
                        ((int64_t)x[extract_rs1(inst)] * (int64_t)x[extract_rs2(inst)]) >> 32;
                    break;
                case FUNCT3_MULHSU:
                    x[extract_rd(inst)] =
                        ((int64_t)x[extract_rs1(inst)] * (uint64_t)x[extract_rs2(inst)]) >> 32;
                    break;
                case FUNCT3_MULHU:
                    x[extract_rd(inst)] =
                        ((uint64_t)x[extract_rs1(inst)] * (uint64_t)x[extract_rs2(inst)]) >> 32;
                    break;
                case FUNCT3_DIV:
                    if (x[extract_rs2(inst)] == 0)
                        x[extract_rd(inst)] = 0xFFFFFFFF;
                    else
                        x[extract_rd(inst)] =
                            (int32_t)x[extract_rs1(inst)] /
                            (int32_t)x[extract_rs2(inst)];
                    break;
                case FUNCT3_DIVU:
                    if (x[extract_rs2(inst)] == 0)
                        x[extract_rd(inst)] = 0xFFFFFFFF;
                    else
                        x[extract_rd(inst)] =
                            x[extract_rs1(inst)] / x[extract_rs2(inst)];
                    break;
                case FUNCT3_REM:
                    if (x[extract_rs2(inst)] == 0)
                        x[extract_rd(inst)] = x[extract_rs1(inst)];
                    else
                        x[extract_rd(inst)] =
                            (int32_t)x[extract_rs1(inst)] %
                            (int32_t)x[extract_rs2(inst)];
                    break;
                case FUNCT3_REMU:
                    if (x[extract_rs2(inst)] == 0)
                        x[extract_rd(inst)] = x[extract_rs1(inst)];
                    else
                        x[extract_rd(inst)] =
                            x[extract_rs1(inst)] % x[extract_rs2(inst)];
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
            break;
        }
        case OP_IMM:
        {
            switch (extract_funct3(inst))
            {
                // Base integer instructions
            case FUNCT3_ADD_SUB:
                // ADDI, there is no SUBI
                x[extract_rd(inst)] =
                    x[extract_rs1(inst)] + extract_imm_i(inst);
                break;
            case FUNCT3_XOR:
                x[extract_rd(inst)] =
                    x[extract_rs1(inst)] ^ extract_imm_i(inst);
                break;
            case FUNCT3_OR:
                x[extract_rd(inst)] =
                    x[extract_rs1(inst)] | extract_imm_i(inst);
                break;
            case FUNCT3_AND:
                x[extract_rd(inst)] =
                    x[extract_rs1(inst)] & extract_imm_i(inst);
                break;
            case FUNCT3_SLL:
                x[extract_rd(inst)] =
                    x[extract_rs1(inst)] << (extract_imm_i(inst) & 0x1F);
                break;
            case FUNCT3_SRL_SRA:
                if ((extract_funct7(inst) & 0x20) == 0)
                    // SRLI
                    x[extract_rd(inst)] =
                        x[extract_rs1(inst)] >> (extract_imm_i(inst) & 0x1F);
                else
                {
                    // SRAI
                    x[extract_rd(inst)] =
                        (int32_t)x[extract_rs1(inst)] >> (extract_imm_i(inst) & 0x1F);
                }
                break;
            case FUNCT3_SLT:
                x[extract_rd(inst)] =
                    (int32_t)x[extract_rs1(inst)] < (int32_t)extract_imm_i(inst);
                break;
            case FUNCT3_SLTU:
                x[extract_rd(inst)] =
                    x[extract_rs1(inst)] < extract_imm_i(inst);
                break;
            default:
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_LOAD:
        {
            switch (extract_funct3(inst))
            {
                // Base load instructions
            case FUNCT3_LB:
            {
                uint8_t value;
                if (read8(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                x[extract_rd(inst)] = sign_extend(value, 8);
            }
            break;
            case FUNCT3_LH:
            {
                uint16_t value;
                if (read16(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                x[extract_rd(inst)] = sign_extend(value, 16);
            }
            break;
            case FUNCT3_LW:
            {
                uint32_t value;
                if (read32(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                x[extract_rd(inst)] = value;
            }
            break;
            case FUNCT3_LBU:
            {
                uint8_t value;
                if (read8(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                x[extract_rd(inst)] = value;
            }
            break;
            case FUNCT3_LHU:
            {
                uint16_t value;
                if (read16(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                x[extract_rd(inst)] = value;
            }
            break;
            default:
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_STORE:
        {
            switch (extract_funct3(inst))
            {
                // Base store instructions
            case FUNCT3_SB:
            {
                uint8_t value = x[extract_rs2(inst)] & 0xFF;
                if (write8(x[extract_rs1(inst)] + extract_imm_s(inst), value) != 0)
                    return -1;
            }
            break;
            case FUNCT3_SH:
            {
                uint16_t value = x[extract_rs2(inst)] & 0xFFFF;
                if (write16(x[extract_rs1(inst)] + extract_imm_s(inst), value) != 0)
                    return -1;
            }
            break;
            case FUNCT3_SW:
            {
                uint32_t value = x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)] + extract_imm_s(inst), value) != 0)
                    return -1;
            }
            break;
            default:
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_BRANCH:
        {
            switch (extract_funct3(inst))
            {
                // Base branch instructions
            case FUNCT3_BEQ:
                if (x[extract_rs1(inst)] == x[extract_rs2(inst)])
                    pc += extract_imm_b(inst) - 4;
                break;
            case FUNCT3_BNE:
                if (x[extract_rs1(inst)] != x[extract_rs2(inst)])
                    pc += extract_imm_b(inst) - 4;
                break;
            case FUNCT3_BLT:
                if ((int32_t)x[extract_rs1(inst)] < (int32_t)x[extract_rs2(inst)])
                    pc += extract_imm_b(inst) - 4;
                break;
            case FUNCT3_BGE:
                if ((int32_t)x[extract_rs1(inst)] >= (int32_t)x[extract_rs2(inst)])
                    pc += extract_imm_b(inst) - 4;
                break;
            case FUNCT3_BLTU:
                if (x[extract_rs1(inst)] < x[extract_rs2(inst)])
                    pc += extract_imm_b(inst) - 4;
                break;
            case FUNCT3_BGEU:
                if (x[extract_rs1(inst)] >= x[extract_rs2(inst)])
                    pc += extract_imm_b(inst) - 4;
                break;
            default:
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_JAL:
        {
            // JAL
            x[extract_rd(inst)] = pc + 4;
            pc += extract_imm_j(inst) - 4;
            break;
        }
        case OP_JALR:
        {
            // JALR
            x[extract_rd(inst)] = pc + 4;
            pc = (x[extract_rs1(inst)] + extract_imm_i(inst)) & ~0x1;
            pc -= 4;
            break;
        }
        case OP_LUI:
        {
            // LUI
            x[extract_rd(inst)] = extract_imm_u(inst);
            break;
        }
        case OP_AUIPC:
        {
            // AUIPC
            x[extract_rd(inst)] = pc + extract_imm_u(inst) - 4;
            break;
        }
        case OP_SYSTEM:
        {
            // System instructions
            switch (extract_funct3(inst))
            {
            case FUNCT3_ECALL_EBREAK:
                if ((extract_funct7(inst) & 0x1) == 0)
                    // ECALL
                    return do_syscall(*this);
                else
                    // EBREAK
                    signal(SIGTRAP);
                break;
            case FUNCT3_CSRRW:
                // CSR Read and Write
                {
                    uint32_t csr = extract_imm_i(inst);
                    switch (csr)
                    {
                    case 0x1: // FP Flags
                        x[extract_rd(inst)] = (fcsr & 0x1F);
                        fcsr &= ~0x1F;
                        fcsr |= (x[extract_rs1(inst)] & 0x1F);
                        break;
                    case 0x2: // FP default round mode
                        x[extract_rd(inst)] = (fcsr >> 5) & 0b111;
                        fcsr &= ~(0b111 << 5);
                        fcsr |= ((x[extract_rs1(inst)] & 0b111) << 5);
                        break;
                    case 0x3: // whole FCSR
                        x[extract_rd(inst)] = fcsr;
                        fcsr = x[extract_rs1(inst)] & 0xFFFFFFFF;
                        break;
                    default:
                        // Unknown CSR
                        signal(SIGILL);
                        return -1;
                    }
                }
                break;
            case FUNCT3_CSRRS:
                // CSR Read and Set
                {
                    uint32_t csr = extract_imm_i(inst);
                    switch (csr)
                    {
                    case 0x1: // FP Flags
                        x[extract_rd(inst)] = (fcsr & 0x1F);
                        fcsr |= (x[extract_rs1(inst)] & 0x1F);
                        break;
                    case 0x2: // FP default round mode
                        x[extract_rd(inst)] = (fcsr >> 5) & 0b111;
                        fcsr |= ((x[extract_rs1(inst)] & 0b111) << 5);
                        break;
                    case 0x3: // whole FCSR
                        x[extract_rd(inst)] = fcsr;
                        fcsr |= (x[extract_rs1(inst)] & 0xFFFFFFFF);
                        break;
                    default:
                        // Unknown CSR
                        signal(SIGILL);
                        return -1;
                    }
                }
                break;
            case FUNCT3_CSRRC:
                // CSR Read and Clear
                {
                    uint32_t csr = extract_imm_i(inst);
                    switch (csr)
                    {
                    case 0x1: // FP Flags
                        x[extract_rd(inst)] = (fcsr & 0x1F);
                        fcsr &= ~(x[extract_rs1(inst)] & 0x1F);
                        break;
                    case 0x2: // FP default round mode
                        x[extract_rd(inst)] = (fcsr >> 5) & 0b111;
                        fcsr &= ~((x[extract_rs1(inst)] & 0b111) << 5);
                        break;
                    case 0x3: // whole FCSR
                        x[extract_rd(inst)] = fcsr;
                        fcsr &= ~(x[extract_rs1(inst)] & 0xFFFFFFFF);
                        break;
                    default:
                        // Unknown CSR
                        signal(SIGILL);
                        return -1;
                    }
                }
                break;
            case FUNCT3_CSRRWI:
                // CSR Read and Write Immediate
                {
                    uint32_t csr = extract_imm_i(inst);
                    switch (csr)
                    {
                    case 0x1: // FP Flags
                        x[extract_rd(inst)] = (fcsr & 0x1F);
                        fcsr &= ~0x1F;
                        fcsr |= (extract_rs1(inst) & 0x1F);
                        break;
                    case 0x2: // FP default round mode
                        x[extract_rd(inst)] = (fcsr >> 5) & 0b111;
                        fcsr &= ~(0b111 << 5);
                        fcsr |= ((extract_rs1(inst) & 0b111) << 5);
                        break;
                    case 0x3: // whole FCSR
                        x[extract_rd(inst)] = fcsr;
                        fcsr = extract_rs1(inst) & 0xFFFFFFFF;
                        break;
                    default:
                        // Unknown CSR
                        signal(SIGILL);
                        return -1;
                    }
                }
                break;
            case FUNCT3_CSRRSI:
                // CSR Read and Set Immediate
                {
                    uint32_t csr = extract_imm_i(inst);
                    switch (csr)
                    {
                    case 0x1: // FP Flags
                        x[extract_rd(inst)] = (fcsr & 0x1F);
                        fcsr |= (extract_rs1(inst) & 0x1F);
                        break;
                    case 0x2: // FP default round mode
                        x[extract_rd(inst)] = (fcsr >> 5) & 0b111;
                        fcsr |= ((extract_rs1(inst) & 0b111) << 5);
                        break;
                    case 0x3: // whole FCSR
                        x[extract_rd(inst)] = fcsr;
                        fcsr |= (extract_rs1(inst) & 0xFFFFFFFF);
                        break;
                    default:
                        // Unknown CSR
                        signal(SIGILL);
                        return -1;
                    }
                }
                break;
            case FUNCT3_CSRRCI:
                // CSR Read and Clear Immediate
                {
                    uint32_t csr = extract_imm_i(inst);
                    switch (csr)
                    {
                    case 0x1: // FP Flags
                        x[extract_rd(inst)] = (fcsr & 0x1F);
                        fcsr &= ~(extract_rs1(inst) & 0x1F);
                        break;
                    case 0x2: // FP default round mode
                        x[extract_rd(inst)] = (fcsr >> 5) & 0b111;
                        fcsr &= ~((extract_rs1(inst) & 0b111) << 5);
                        break;
                    case 0x3: // whole FCSR
                        x[extract_rd(inst)] = fcsr;
                        fcsr &= ~(extract_rs1(inst) & 0xFFFFFFFF);
                        break;
                    default:
                        // Unknown CSR
                        signal(SIGILL);
                        return -1;
                    }
                }
                break;
            default:
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_MISC_MEM:
        {
            // FENCE
            if (extract_funct3(inst) == FUNCT3_FENCE)
            {
                // No operation
            }
            else if (extract_funct3(inst) == FUNCT3_FENCE_I)
            {
                // FENCE.I
                // No operation
            }
            else
            {
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_ATOMIC:
        {
            if (extract_funct3(inst) != 0x2)
            {
                // Unknown funct3
                signal(SIGILL);
                return -1;
            }
            switch (extract_funct5(inst))
            {
            case FUNCT5_LR:
            {
                // Load Reserved
                uint32_t val;
                if (read32(x[extract_rs1(inst)], val) != 0)
                    return -1;
                process->reserved_mem[x[extract_rs1(inst)]] = id;
                x[extract_rd(inst)] = val;
                break;
            }
            case FUNCT5_SC:
            {
                // Store Conditional
                auto it = process->reserved_mem.find(x[extract_rs1(inst)]);
                if (it == process->reserved_mem.end() || it->second != id)
                {
                    // Not reserved
                    x[extract_rd(inst)] = 1;
                    break;
                }
                uint32_t val = x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)], val) != 0)
                    return -1;
                process->reserved_mem.erase(it);
                x[extract_rd(inst)] = 0;
                break;
            }
            case FUNCT5_AMOSWAP:
            {
                // Atomic Swap
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOADD:
            {
                // Atomic Add
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = old_val + x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOAND:
            {
                // Atomic AND
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = old_val & x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOOR:
            {
                // Atomic OR
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = old_val | x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOXOR:
            {
                // Atomic XOR
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = old_val ^ x[extract_rs2(inst)];
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOMAX:
            {
                // Atomic Max
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = std::max((int32_t)old_val, (int32_t)x[extract_rs2(inst)]);
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOMIN:
            {
                // Atomic Min
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = std::min((int32_t)old_val, (int32_t)x[extract_rs2(inst)]);
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOMAXU:
            {
                // Atomic Max Unsigned
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = std::max(old_val, x[extract_rs2(inst)]);
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            case FUNCT5_AMOMINU:
            {
                // Atomic Min Unsigned
                uint32_t old_val;
                if (read32(x[extract_rs1(inst)], old_val) != 0)
                    return -1;
                uint32_t new_val = std::min(old_val, x[extract_rs2(inst)]);
                if (write32(x[extract_rs1(inst)], new_val) != 0)
                    return -1;
                x[extract_rd(inst)] = old_val;
                break;
            }
            default:
                // Unknown funct5
                signal(SIGILL);
                return -1;
            }
            break;
        }
        case OP_FLW:
        {
            set_round_mode(ROUND_DYN, fcsr);
            if (extract_funct3(inst) & 0x1)
            {
                // FLD
                double value;
                if (readf64(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                f[extract_rd(inst)] = value;
            }
            else
            {
                // FLW
                float value;
                if (readf32(x[extract_rs1(inst)] + extract_imm_i(inst), value) != 0)
                    return -1;
                write_float_to_double(value, f[extract_rd(inst)]);
            }
            break;
        }
        case OP_FSW:
        {
            set_round_mode(ROUND_DYN, fcsr);
            if (extract_funct3(inst) & 0x1)
            {
                // FSD
                if (writef64(x[extract_rs1(inst)] + extract_imm_s(inst), f[extract_rs2(inst)]) != 0)
                    return -1;
            }
            else
            {
                // FSW
                float value = read_float_from_double(f[extract_rs2(inst)]);
                if (writef32(x[extract_rs1(inst)] + extract_imm_s(inst), value) != 0)
                    return -1;
            }
            break;
        }
        case OP_FMADD:
        {
            // FMADD
            set_round_mode(extract_funct3(inst), fcsr);
            if (is_double_precision(inst))
            {
                double a = f[extract_rs1(inst)];
                double b = f[extract_rs2(inst)];
                double c = f[extract_rs3(inst)];
                f[extract_rd(inst)] = a * b + c;
            }
            else
            {
                float a = read_float_from_double(f[extract_rs1(inst)]);
                float b = read_float_from_double(f[extract_rs2(inst)]);
                float c = read_float_from_double(f[extract_rs3(inst)]);
                write_float_to_double(a * b + c, f[extract_rd(inst)]);
            }
            break;
        }
        case OP_FMSUB:
        {
            // FMSUB
            set_round_mode(extract_funct3(inst), fcsr);
            if (is_double_precision(inst))
            {
                double a = f[extract_rs1(inst)];
                double b = f[extract_rs2(inst)];
                double c = f[extract_rs3(inst)];
                f[extract_rd(inst)] = a * b - c;
            }
            else
            {
                float a = read_float_from_double(f[extract_rs1(inst)]);
                float b = read_float_from_double(f[extract_rs2(inst)]);
                float c = read_float_from_double(f[extract_rs3(inst)]);
                write_float_to_double(a * b - c, f[extract_rd(inst)]);
            }
            break;
        }
        case OP_FNMADD:
        {
            // FNMADD
            set_round_mode(extract_funct3(inst), fcsr);
            if (is_double_precision(inst))
            {
                double a = f[extract_rs1(inst)];
                double b = f[extract_rs2(inst)];
                double c = f[extract_rs3(inst)];
                f[extract_rd(inst)] = -(a * b) - c;
            }
            else
            {
                float a = read_float_from_double(f[extract_rs1(inst)]);
                float b = read_float_from_double(f[extract_rs2(inst)]);
                float c = read_float_from_double(f[extract_rs3(inst)]);
                write_float_to_double(-(a * b) - c, f[extract_rd(inst)]);
            }
            break;
        }
        case OP_FNMSUB:
        {
            // FNMSUB
            set_round_mode(extract_funct3(inst), fcsr);
            if (is_double_precision(inst))
            {
                double a = f[extract_rs1(inst)];
                double b = f[extract_rs2(inst)];
                double c = f[extract_rs3(inst)];
                f[extract_rd(inst)] = -(a * b) + c;
            }
            else
            {
                float a = read_float_from_double(f[extract_rs1(inst)]);
                float b = read_float_from_double(f[extract_rs2(inst)]);
                float c = read_float_from_double(f[extract_rs3(inst)]);
                write_float_to_double(-(a * b) + c, f[extract_rd(inst)]);
            }
            break;
        }
        case OP_FREG:
        {
            float a, b;
            double ad, bd;
            switch (extract_funct7(inst))
            {
            case 0b0000000:
                // FADD.S
                set_round_mode(extract_funct3(inst), fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                write_float_to_double(a + b, f[extract_rd(inst)]);
                break;
            case 0b0000100:
                // FSUB.S
                set_round_mode(extract_funct3(inst), fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                write_float_to_double(a - b, f[extract_rd(inst)]);
                break;
            case 0b0001000:
                // FMUL.S
                set_round_mode(extract_funct3(inst), fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                write_float_to_double(a * b, f[extract_rd(inst)]);
                break;
            case 0b0001100:
                // FDIV.S
                set_round_mode(extract_funct3(inst), fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                write_float_to_double(a / b, f[extract_rd(inst)]);
                break;
            case 0b0101100:
                // FSQRT.S
                set_round_mode(extract_funct3(inst), fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                write_float_to_double(sqrt(a), f[extract_rd(inst)]);
                break;
            case 0b0010000:
                // FSGN*.S
                set_round_mode(ROUND_DYN, fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                switch (extract_funct3(inst))
                {
                case 0b000:
                    // FSGNJ.S
                    write_float_to_double(b < 0 ? -abs(a) : abs(a),
                                          f[extract_rd(inst)]);
                    break;
                case 0b001:
                    // FSGNJN.S
                    write_float_to_double(b < 0 ? abs(a) : -abs(a),
                                          f[extract_rd(inst)]);
                    break;
                case 0b010:
                    // FSGNJX.S
                    write_float_to_double(b < 0 ? -a : a,
                                          f[extract_rd(inst)]);
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b0010100:
                // F(MIN|MAX).S
                set_round_mode(ROUND_DYN, fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                if (extract_funct3(inst) == 0b000)
                    // FMIN.S
                    write_float_to_double(std::min(a, b), f[extract_rd(inst)]);
                else if (extract_funct3(inst) == 0b001)
                    // FMAX.S
                    write_float_to_double(std::max(a, b), f[extract_rd(inst)]);
                else
                {
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1100000:
                // FCVT.W*.S
                set_round_mode(extract_funct3(inst), fcsr);
                switch (extract_rs2(inst))
                {
                case 0b00000:
                    // FCVT.W.S
                    a = read_float_from_double(f[extract_rs1(inst)]);
                    if (a > INT32_MAX || a < INT32_MIN)
                    {
                        // Overflow
                        signal(SIGFPE);
                        return -1;
                    }
                    x[extract_rd(inst)] = (int32_t)a;
                    break;
                case 0b00001:
                    // FCVT.WU.S
                    a = read_float_from_double(f[extract_rs1(inst)]);
                    if (a > UINT32_MAX || a < 0)
                    {
                        // Overflow
                        signal(SIGFPE);
                        return -1;
                    }
                    x[extract_rd(inst)] = (uint32_t)a;
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1110000:
                // FMV.X.W or FCLASS.S
                set_round_mode(ROUND_DYN, fcsr);
                switch (extract_funct3(inst))
                {
                case 0b000:
                    // FMV.X.W
                    a = read_float_from_double(f[extract_rs1(inst)]);
                    memcpy(&x[extract_rd(inst)], &a, sizeof(float));
                    break;
                case 0b001:
                    // FCLASS.S
                    a = read_float_from_double(f[extract_rs1(inst)]);
                    x[extract_rd(inst)] = classify_float(a);
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1010000:
                // FEQ.S or FLT.S or FLE.S
                set_round_mode(ROUND_DYN, fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                b = read_float_from_double(f[extract_rs2(inst)]);
                switch (extract_funct3(inst))
                {
                case 0b000:
                    // FEQ.S
                    x[extract_rd(inst)] = (a == b);
                    break;
                case 0b001:
                    // FLT.S
                    x[extract_rd(inst)] = (a < b);
                    break;
                case 0b010:
                    // FLE.S
                    x[extract_rd(inst)] = (a <= b);
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1101000:
                // FCVT.S.W*
                set_round_mode(extract_funct3(inst), fcsr);
                switch (extract_rs2(inst))
                {
                case 0b00000:
                    // FCVT.S.W
                    a = (float)(int32_t)x[extract_rs1(inst)];
                    write_float_to_double(a, f[extract_rd(inst)]);
                    break;
                case 0b00001:
                    // FCVT.S.WU
                    a = (float)(uint32_t)x[extract_rs1(inst)];
                    write_float_to_double(a, f[extract_rd(inst)]);
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1111000:
                // FMV.W.X
                set_round_mode(ROUND_DYN, fcsr);
                a = read_float_from_double(f[extract_rs1(inst)]);
                memcpy(&x[extract_rd(inst)], &a, sizeof(float));
                break;
            case 0b0000001:
                // FADD.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                f[extract_rd(inst)] = ad + bd;
                break;
            case 0b0000101:
                // FSUB.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                f[extract_rd(inst)] = ad - bd;
                break;
            case 0b0001001:
                // FMUL.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                f[extract_rd(inst)] = ad * bd;
                break;
            case 0b0001101:
                // FDIV.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                f[extract_rd(inst)] = ad / bd;
                break;
            case 0b0101101:
                // FSQRT.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                f[extract_rd(inst)] = sqrt(ad);
                break;
            case 0b0010001:
                // FSGN*.D
                set_round_mode(ROUND_DYN, fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                switch (extract_funct3(inst))
                {
                case 0b000:
                    // FSGNJ.D
                    f[extract_rd(inst)] = bd < 0 ? -abs(ad) : abs(ad);
                    break;
                case 0b001:
                    // FSGNJN.D
                    f[extract_rd(inst)] = bd < 0 ? abs(ad) : -abs(ad);
                    break;
                case 0b010:
                    // FSGNJX.D
                    f[extract_rd(inst)] = bd < 0 ? -ad : ad;
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b0010101:
                // F(MIN|MAX).D
                set_round_mode(ROUND_DYN, fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                if (extract_funct3(inst) == 0b000)
                    // FMIN.D
                    f[extract_rd(inst)] = std::min(ad, bd);
                else if (extract_funct3(inst) == 0b001)
                    // FMAX.D
                    f[extract_rd(inst)] = std::max(ad, bd);
                else
                {
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b0100000:
                // FCVT.S.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                write_float_to_double(ad, f[extract_rd(inst)]);
                break;
            case 0b0100001:
                // FCVT.D.S
                set_round_mode(extract_funct3(inst), fcsr);
                ad = read_float_from_double(f[extract_rs1(inst)]);
                f[extract_rd(inst)] = ad;
                break;
            case 0b1010001:
                // FEQ.D or FLT.D or FLE.D
                set_round_mode(ROUND_DYN, fcsr);
                ad = f[extract_rs1(inst)];
                bd = f[extract_rs2(inst)];
                switch (extract_funct3(inst))
                {
                case 0b000:
                    // FEQ.D
                    x[extract_rd(inst)] = (ad == bd);
                    break;
                case 0b001:
                    // FLT.D
                    x[extract_rd(inst)] = (ad < bd);
                    break;
                case 0b010:
                    // FLE.D
                    x[extract_rd(inst)] = (ad <= bd);
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1110001:
                // FCLASS.D
                set_round_mode(ROUND_DYN, fcsr);
                ad = f[extract_rs1(inst)];
                x[extract_rd(inst)] = classify_double(ad);
                break;
            case 0b1100001:
                // FCVT.W.D or FCVT.WU.D
                set_round_mode(extract_funct3(inst), fcsr);
                ad = f[extract_rs1(inst)];
                switch (extract_rs2(inst))
                {
                case 0b00000:
                    // FCVT.W.D
                    if (ad > INT32_MAX || ad < INT32_MIN)
                    {
                        // Overflow
                        signal(SIGFPE);
                        return -1;
                    }
                    x[extract_rd(inst)] = (int32_t)std::nearbyint(ad);
                    break;
                case 0b00001:
                    // FCVT.WU.D
                    if (ad > UINT32_MAX || ad < 0)
                    {
                        // Overflow
                        signal(SIGFPE);
                        return -1;
                    }
                    x[extract_rd(inst)] = (uint32_t)std::nearbyint(ad);
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
                break;
            case 0b1101001:
                // FCVT.D.W*
                set_round_mode(extract_funct3(inst), fcsr);
                switch (extract_rs2(inst))
                {
                case 0b00000:
                    // FCVT.D.W
                    ad = (double)(int32_t)x[extract_rs1(inst)];
                    f[extract_rd(inst)] = ad;
                    break;
                case 0b00001:
                    // FCVT.D.WU
                    ad = (double)(uint32_t)x[extract_rs1(inst)];
                    f[extract_rd(inst)] = ad;
                    break;
                default:
                    // Unknown funct3
                    signal(SIGILL);
                    return -1;
                }
            }
            break;
        }
        default:
            // Unknown opcode
            signal(SIGILL);
            return -1;
        }

        return 0;
    }

    void Thread::handle_signal()
    {
        // TODO: Implement signal handling
        // For now, just stop
        state = ThreadState::ENDED;
        printf("Signal %d received\n", pending_signal);
    }
} // namespace Hamster
