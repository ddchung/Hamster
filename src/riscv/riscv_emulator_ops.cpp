// opcode implementation

#include <riscv/riscv_emulator.hpp>
#include <platform/config.hpp>
#include <math.h>
#include <cstring>

namespace Hamster
{
    namespace
    {
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
            if (value & (1U << (bits - 1))) // Use 1U to avoid signed shift
            {
                value |= ~((1U << bits) - 1);
            }
            return value;
        }

        uint32_t extract_opcode(uint32_t inst)
        {
            return (inst & 0x7F) >> 2;
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

        bool is_double_precision(uint32_t funct7)
        {
            return (funct7 & 0b11) == 0b01;
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

    // Computed goto's are a GNU extension, but they are supported by both GCC and Clang.
    // Note that we use them here to implement direct threading, which is not the same as a switch-case dispatch due
    // to its branch prediction friendliness and lower overhead.
    // Thus, we'll disable -Wpedantic.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#define DISPATCH()                                                               \
    do                                                                           \
    {                                                                            \
        x[0] = 0;                                                                \
        pc += 4;                                                                 \
        if HAMSTER_UNLIKELY (++current_inst >= predecoded_insts + decoded_count) \
            return current_inst - predecoded_insts;                              \
        goto * current_inst->handler;                                            \
    } while (0)

#define OPCODE_RETURN_OK()                          \
    do                                              \
    {                                               \
        pc += 4;                                    \
        return current_inst - predecoded_insts + 1; \
    } while (0)

#define OPCODE_RETURN_FAIL() return current_inst - predecoded_insts
    __attribute__((flatten))
    uint32_t RiscVEmulator::execute_trace(ExecuteResult &result)
    {
        uint32_t prefetched_instructions[HAMSTER_TRACE_SIZE];
        DecodedInst predecoded_insts[HAMSTER_TRACE_SIZE];

        static constexpr void *opcode_jumptable[] =
        {
            &&case_op_load,
            &&case_op_flw,
            &&case_invalid_op,
            &&case_op_misc_mem,
            &&case_op_imm,
            &&case_op_auipc,
            &&case_invalid_op,
            &&case_invalid_op,
            &&case_op_store,
            &&case_op_fsw,
            &&case_invalid_op,
            &&case_op_atomic,
            &&case_op_reg,
            &&case_op_lui,
            &&case_invalid_op,
            &&case_invalid_op,
            &&case_op_fmadd,
            &&case_op_fmsub,
            &&case_op_fnmsub,
            &&case_op_fnmadd,
            &&case_op_freg,
            &&case_invalid_op,
            &&case_invalid_op,
            &&case_invalid_op,
            &&case_op_branch,
            &&case_op_jalr,
            &&case_invalid_op,
            &&case_op_jal,
            &&case_op_system,
            &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
            // &&case_invalid_op, &&case_invalid_op, &&case_invalid_op, &&case_invalid_op,
        };

        // Fetch instructions
        if (pc & 0b11)
        {
            // Unaligned fetch not allowed
            result.status = ExecuteResult::Status::IllegalLoad;
            result.illegal_load.address = pc;
            return 0;
        }
        if (memory->memory.fast_fetch_trace(pc, prefetched_instructions) < 0)
        {
            result.status = ExecuteResult::Status::IllegalLoad;
            result.illegal_load.address = pc;
            return 0;
        }

        size_t decoded_count = 0;
        for (uint32_t *p_inst = prefetched_instructions; p_inst < prefetched_instructions + HAMSTER_TRACE_SIZE; ++p_inst)
        {
            uint32_t inst = *p_inst;

            // Decode instruction
            DecodedInst &dinst = predecoded_insts[decoded_count];
            dinst.handler = opcode_jumptable[extract_opcode(inst)];
            dinst.imm_i = extract_imm_i(inst);
            dinst.imm_s = extract_imm_s(inst);
            dinst.imm_b = extract_imm_b(inst);
            dinst.imm_u = extract_imm_u(inst);
            dinst.imm_j = extract_imm_j(inst);
            dinst.rd = extract_rd(inst);
            dinst.funct3 = extract_funct3(inst);
            dinst.rs1 = extract_rs1(inst);
            dinst.rs2 = extract_rs2(inst);
            dinst.funct7 = extract_funct7(inst);
            ++decoded_count;

            if (dinst.handler == &&case_op_branch || dinst.handler == &&case_op_jalr  ||
                dinst.handler == &&case_op_jal || decoded_count == HAMSTER_TRACE_SIZE)
            {
                // Control flow change, end of trace
                break;
            }
        }

        DecodedInst *current_inst = predecoded_insts;
        uint32_t new_pc;
        uint32_t dummy;

        if (decoded_count == 0)
            return 0;
        x[0] = 0;
        goto * current_inst->handler;

    case_op_reg:
        if ((current_inst->funct7 & 0x1) == 0)
            switch (current_inst->funct3)
            {
                // Base integer instructions
            case FUNCT3_ADD_SUB:
                if ((current_inst->funct7 & 0x20) == 0)
                    // ADD
                    x[current_inst->rd] =
                        x[current_inst->rs1] + x[current_inst->rs2];
                else
                    // SUB
                    x[current_inst->rd] =
                        x[current_inst->rs1] - x[current_inst->rs2];
                break;
            case FUNCT3_XOR:
                x[current_inst->rd] =
                    x[current_inst->rs1] ^ x[current_inst->rs2];
                break;
            case FUNCT3_OR:
                x[current_inst->rd] =
                    x[current_inst->rs1] | x[current_inst->rs2];
                break;
            case FUNCT3_AND:
                x[current_inst->rd] =
                    x[current_inst->rs1] & x[current_inst->rs2];
                break;
            case FUNCT3_SLL:
                x[current_inst->rd] =
                    x[current_inst->rs1] << (x[current_inst->rs2] & 0x1F);
                break;
            case FUNCT3_SRL_SRA:
                if ((current_inst->funct7 & 0x20) == 0)
                    // SRL
                    x[current_inst->rd] =
                        x[current_inst->rs1] >> (x[current_inst->rs2] & 0x1F);
                else
                {
                    // SRA
                    x[current_inst->rd] = (int32_t)x[current_inst->rs1] >> (x[current_inst->rs2] & 0x1F);
                }
                break;
            case FUNCT3_SLT:
                x[current_inst->rd] =
                    (int32_t)x[current_inst->rs1] < (int32_t)x[current_inst->rs2];
                break;
            case FUNCT3_SLTU:
                x[current_inst->rd] =
                    x[current_inst->rs1] < x[current_inst->rs2];
                break;
            default:
                // Unknown funct3
                _trace("RiscVEmulator: Unknown funct3 for OP_REG, instruction: 0x%08x\n", current_inst->inst);
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
        else
            switch (current_inst->funct3)
            {
                // -M extension instructions
            case FUNCT3_MUL:
                x[current_inst->rd] =
                    (int64_t)x[current_inst->rs1] * (int64_t)x[current_inst->rs2];
                break;
            case FUNCT3_MULH:
                x[current_inst->rd] =
                    ((int64_t)x[current_inst->rs1] * (int64_t)x[current_inst->rs2]) >> 32;
                break;
            case FUNCT3_MULHSU:
                x[current_inst->rd] =
                    ((int64_t)x[current_inst->rs1] * (uint64_t)x[current_inst->rs2]) >> 32;
                break;
            case FUNCT3_MULHU:
                x[current_inst->rd] =
                    ((uint64_t)x[current_inst->rs1] * (uint64_t)x[current_inst->rs2]) >> 32;
                break;
            case FUNCT3_DIV:
                if (x[current_inst->rs2] == 0)
                    x[current_inst->rd] = 0xFFFFFFFF;
                else
                    x[current_inst->rd] =
                        (int32_t)x[current_inst->rs1] /
                        (int32_t)x[current_inst->rs2];
                break;
            case FUNCT3_DIVU:
                if (x[current_inst->rs2] == 0)
                    x[current_inst->rd] = 0xFFFFFFFF;
                else
                    x[current_inst->rd] =
                        x[current_inst->rs1] / x[current_inst->rs2];
                break;
            case FUNCT3_REM:
                if (x[current_inst->rs2] == 0)
                    x[current_inst->rd] = x[current_inst->rs1];
                else
                    x[current_inst->rd] =
                        (int32_t)x[current_inst->rs1] %
                        (int32_t)x[current_inst->rs2];
                break;
            case FUNCT3_REMU:
                if (x[current_inst->rs2] == 0)
                    x[current_inst->rd] = x[current_inst->rs1];
                else
                    x[current_inst->rd] =
                        x[current_inst->rs1] % x[current_inst->rs2];
                break;
            default:
                // Unknown funct3
                _trace("RiscVEmulator: Unknown funct3 for OP_REG, instruction: 0x%08x\n", current_inst->inst);
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
        DISPATCH();

    case_op_imm:
        switch (current_inst->funct3)
        {
            // Base integer instructions
        case FUNCT3_ADD_SUB:
            // ADDI, there is no SUBI
            x[current_inst->rd] =
                x[current_inst->rs1] + current_inst->imm_i;
            break;
        case FUNCT3_XOR:
            x[current_inst->rd] =
                x[current_inst->rs1] ^ current_inst->imm_i;
            break;
        case FUNCT3_OR:
            x[current_inst->rd] =
                x[current_inst->rs1] | current_inst->imm_i;
            break;
        case FUNCT3_AND:
            x[current_inst->rd] =
                x[current_inst->rs1] & current_inst->imm_i;
            break;
        case FUNCT3_SLL:
            x[current_inst->rd] =
                x[current_inst->rs1] << (current_inst->imm_i & 0x1F);
            break;
        case FUNCT3_SRL_SRA:
            if ((current_inst->funct7 & 0x20) == 0)
                // SRLI
                x[current_inst->rd] =
                    x[current_inst->rs1] >> (current_inst->imm_i & 0x1F);
            else
            {
                // SRAI
                x[current_inst->rd] =
                    (int32_t)x[current_inst->rs1] >> (current_inst->imm_i & 0x1F);
            }
            break;
        case FUNCT3_SLT:
            x[current_inst->rd] =
                (int32_t)x[current_inst->rs1] < (int32_t)current_inst->imm_i;
            break;
        case FUNCT3_SLTU:
            x[current_inst->rd] =
                x[current_inst->rs1] < current_inst->imm_i;
            break;
        default:
            // Unknown funct3
            _trace("RiscVEmulator: Unknown funct3 for OP_IMM, instruction: 0x%08x\n", current_inst->inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        DISPATCH();

    case_op_load:
        switch (current_inst->funct3)
        {
            // Base load instructions
        case FUNCT3_LB:
        {
            uint8_t value;
            if (read8(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                _trace("RiscVEmulator: LB: Failed to load 8-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_i);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = sign_extend(value, 8);
        }
        break;
        case FUNCT3_LH:
        {
            uint16_t value;
            if (read16(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                _trace("RiscVEmulator: LH: Failed to load 16-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_i);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = sign_extend(value, 16);
        }
        break;
        case FUNCT3_LW:
        {
            uint32_t value;
            if (read32(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                _trace("RiscVEmulator: LW: Failed to load 32-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_i);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = value;
        }
        break;
        case FUNCT3_LBU:
        {
            uint8_t value;
            if (read8(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                _trace("RiscVEmulator: LBU: Failed to load 8-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_i);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = value;
        }
        break;
        case FUNCT3_LHU:
        {
            uint16_t value;
            if (read16(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                _trace("RiscVEmulator: LHU: Failed to load 16-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_i);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = value;
        }
        break;
        default:
            // Unknown funct3
            _trace("RiscVEmulator: Unknown funct3 for OP_LOAD, instruction: 0x%08x\n", current_inst->inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        DISPATCH();

    case_op_store:
        switch (current_inst->funct3)
        {
            // Base store instructions
        case FUNCT3_SB:
        {
            uint8_t value = x[current_inst->rs2] & 0xFF;
            if (write8(x[current_inst->rs1] + current_inst->imm_s, value) != 0)
            {
                _trace("RiscVEmulator: SB: Failed to store 8-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_s);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1] + current_inst->imm_s;
                result.illegal_store.value = value;
                OPCODE_RETURN_FAIL();
            }
        }
        break;
        case FUNCT3_SH:
        {
            uint16_t value = x[current_inst->rs2] & 0xFFFF;
            if (write16(x[current_inst->rs1] + current_inst->imm_s, value) != 0)
            {
                _trace("RiscVEmulator: SH: Failed to store 16-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_s);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1] + current_inst->imm_s;
                result.illegal_store.value = value;
                OPCODE_RETURN_FAIL();
            }
        }
        break;
        case FUNCT3_SW:
        {
            uint32_t value = x[current_inst->rs2];
            if (write32(x[current_inst->rs1] + current_inst->imm_s, value) != 0)
            {
                _trace("RiscVEmulator: SW: Failed to store 32-bit value at 0x%08x\n", x[current_inst->rs1] + current_inst->imm_s);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1] + current_inst->imm_s;
                result.illegal_store.value = value;
                OPCODE_RETURN_FAIL();
            }
        }
        break;
        default:
            // Unknown funct3
            _trace("RiscVEmulator: Unknown funct3 for OP_STORE, instruction: 0x%08x\n", current_inst->inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        DISPATCH();

    case_op_branch:
        switch (current_inst->funct3)
        {
            // Base branch instructions
        case FUNCT3_BEQ:
            if (x[current_inst->rs1] == x[current_inst->rs2])
            {
                auto new_pc = pc + current_inst->imm_b;
                // Ensure that it is readable
                if (read32(new_pc, dummy) != 0)
                {
                    _trace("RiscVEmulator: BEQ: Branch to unreadable address 0x%08x\n", new_pc);
                    result.status = ExecuteResult::Status::IllegalLoad;
                    result.illegal_load.address = new_pc;
                    OPCODE_RETURN_FAIL();
                }
                pc = new_pc;
            }
            else
            {
                pc += 4;
            }
            break;
        case FUNCT3_BNE:
            if (x[current_inst->rs1] != x[current_inst->rs2])
            {
                auto new_pc = pc + current_inst->imm_b;
                // Ensure that it is readable
                if (read32(new_pc, dummy) != 0)
                {
                    _trace("RiscVEmulator: BNE: Branch to unreadable address 0x%08x\n", new_pc);
                    result.status = ExecuteResult::Status::IllegalLoad;
                    result.illegal_load.address = new_pc;
                    OPCODE_RETURN_FAIL();
                }
                pc = new_pc;
            }
            else
            {
                pc += 4;
            }
            break;
        case FUNCT3_BLT:
            if ((int32_t)x[current_inst->rs1] < (int32_t)x[current_inst->rs2])
            {
                auto new_pc = pc + current_inst->imm_b;
                // Ensure that it is readable
                if (read32(new_pc, dummy) != 0)
                {
                    _trace("RiscVEmulator: BLT: Branch to unreadable address 0x%08x\n", new_pc);
                    result.status = ExecuteResult::Status::IllegalLoad;
                    result.illegal_load.address = new_pc;
                    OPCODE_RETURN_FAIL();
                }
                pc = new_pc;
            }
            else
            {
                pc += 4;
            }
            break;
        case FUNCT3_BGE:
            if ((int32_t)x[current_inst->rs1] >= (int32_t)x[current_inst->rs2])
            {
                auto new_pc = pc + current_inst->imm_b;
                // Ensure that it is readable
                if (read32(new_pc, dummy) != 0)
                {
                    _trace("RiscVEmulator: BGE: Branch to unreadable address 0x%08x\n", new_pc);
                    result.status = ExecuteResult::Status::IllegalLoad;
                    result.illegal_load.address = new_pc;
                    OPCODE_RETURN_FAIL();
                }
                pc = new_pc;
            }
            else
            {
                pc += 4;
            }
            break;
        case FUNCT3_BLTU:
            if (x[current_inst->rs1] < x[current_inst->rs2])
            {
                auto new_pc = pc + current_inst->imm_b;
                // Ensure that it is readable
                if (read32(new_pc, dummy) != 0)
                {
                    _trace("RiscVEmulator: BLTU: Branch to unreadable address 0x%08x\n", new_pc);
                    result.status = ExecuteResult::Status::IllegalLoad;
                    result.illegal_load.address = new_pc;
                    OPCODE_RETURN_FAIL();
                }
                pc = new_pc;
            }
            else
            {
                pc += 4;
            }
            break;
        case FUNCT3_BGEU:
            if (x[current_inst->rs1] >= x[current_inst->rs2])
            {
                auto new_pc = pc + current_inst->imm_b;
                // Ensure that it is readable
                if (read32(new_pc, dummy) != 0)
                {
                    _trace("RiscVEmulator: BGEU: Branch to unreadable address 0x%08x\n", new_pc);
                    result.status = ExecuteResult::Status::IllegalLoad;
                    result.illegal_load.address = new_pc;
                    OPCODE_RETURN_FAIL();
                }
                pc = new_pc;
            }
            else
            {
                pc += 4;
            }
            break;
        default:
            // Unknown funct3
            _trace("RiscVEmulator: Unknown funct3 for OP_BRANCH, instruction: 0x%08x\n", current_inst->inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }

        assert(current_inst == predecoded_insts + decoded_count - 1);
        ++total_instructions_executed;
        return decoded_count;

    case_op_jal:
        // JAL
        new_pc = pc + current_inst->imm_j;
        // Ensure that it is readable
        if (read32(new_pc, dummy) != 0)
        {
            _trace("RiscVEmulator: JAL: Jump to unreadable address 0x%08x\n", new_pc);
            result.status = ExecuteResult::Status::IllegalLoad;
            result.illegal_load.address = new_pc;
            OPCODE_RETURN_FAIL();
        }
        x[current_inst->rd] = pc + 4;
        pc = new_pc;

        assert(current_inst == predecoded_insts + decoded_count - 1);
        ++total_instructions_executed;
        return decoded_count;

    case_op_jalr:
        // JALR
        new_pc = (x[current_inst->rs1] + current_inst->imm_i) & ~0x1;
        // Ensure that it is readable
        if (read32(new_pc, dummy) != 0)
        {
            _trace("RiscVEmulator: JALR: Jump to unreadable address 0x%08x\n", new_pc);
            result.status = ExecuteResult::Status::IllegalLoad;
            result.illegal_load.address = new_pc;
            OPCODE_RETURN_FAIL();
        }
        x[current_inst->rd] = pc + 4;
        pc = new_pc;
        assert(current_inst == predecoded_insts + decoded_count - 1);
        ++total_instructions_executed;
        return decoded_count;

    case_op_lui:
        // LUI
        x[current_inst->rd] = current_inst->imm_u;
        DISPATCH();
    case_op_auipc:
        // AUIPC
        x[current_inst->rd] = pc + current_inst->imm_u;
        DISPATCH();
    case_op_system:
        // System instructions
        switch (current_inst->funct3)
        {
        case FUNCT3_ECALL_EBREAK:
            if ((current_inst->imm_i & 0x1) == 0)
            {
                // ECALL
                result.status = ExecuteResult::Status::ECALL;
                OPCODE_RETURN_OK();
            }
            else
            {
                // EBREAK
                result.status = ExecuteResult::Status::EBREAK;
                OPCODE_RETURN_OK();
            }
            break;
        case FUNCT3_CSRRW:
            // CSR Read and Write
            {
                uint32_t csr = current_inst->imm_i;
                switch (csr)
                {
                case 0x1: // FP Flags
                    x[current_inst->rd] = (fcsr & 0x1F);
                    fcsr &= ~0x1F;
                    fcsr |= (x[current_inst->rs1] & 0x1F);
                    break;
                case 0x2: // FP default round mode
                    x[current_inst->rd] = (fcsr >> 5) & 0b111;
                    fcsr &= ~(0b111 << 5);
                    fcsr |= ((x[current_inst->rs1] & 0b111) << 5);
                    break;
                case 0x3: // whole FCSR
                    x[current_inst->rd] = fcsr;
                    fcsr = x[current_inst->rs1] & 0xFFFFFFFF;
                    break;
                default:
                    // Unknown CSR
                    _trace("RiscVEmulator: Unknown CSR access, instruction: 0x%08x\n", current_inst->inst);
                    result.status = ExecuteResult::Status::IllegalInstruction;
                    result.illegal_instruction.instruction = current_inst->inst;
                    OPCODE_RETURN_FAIL();
                }
            }
            break;
        case FUNCT3_CSRRS:
            // CSR Read and Set
            {
                uint32_t csr = current_inst->imm_i;
                switch (csr)
                {
                case 0x1: // FP Flags
                    x[current_inst->rd] = (fcsr & 0x1F);
                    fcsr |= (x[current_inst->rs1] & 0x1F);
                    break;
                case 0x2: // FP default round mode
                    x[current_inst->rd] = (fcsr >> 5) & 0b111;
                    fcsr |= ((x[current_inst->rs1] & 0b111) << 5);
                    break;
                case 0x3: // whole FCSR
                    x[current_inst->rd] = fcsr;
                    fcsr |= (x[current_inst->rs1] & 0xFFFFFFFF);
                    break;
                default:
                    // Unknown CSR
                    _trace("RiscVEmulator: Unknown CSR access, instruction: 0x%08x\n", current_inst->inst);
                    result.status = ExecuteResult::Status::IllegalInstruction;
                    result.illegal_instruction.instruction = current_inst->inst;
                    OPCODE_RETURN_FAIL();
                }
            }
            break;
        case FUNCT3_CSRRC:
            // CSR Read and Clear
            {
                uint32_t csr = current_inst->imm_i;
                switch (csr)
                {
                case 0x1: // FP Flags
                    x[current_inst->rd] = (fcsr & 0x1F);
                    fcsr &= ~(x[current_inst->rs1] & 0x1F);
                    break;
                case 0x2: // FP default round mode
                    x[current_inst->rd] = (fcsr >> 5) & 0b111;
                    fcsr &= ~((x[current_inst->rs1] & 0b111) << 5);
                    break;
                case 0x3: // whole FCSR
                    x[current_inst->rd] = fcsr;
                    fcsr &= ~(x[current_inst->rs1] & 0xFFFFFFFF);
                    break;
                default:
                    // Unknown CSR
                    _trace("RiscVEmulator: Unknown CSR access, instruction: 0x%08x\n", current_inst->inst);
                    result.status = ExecuteResult::Status::IllegalInstruction;
                    result.illegal_instruction.instruction = current_inst->inst;
                    OPCODE_RETURN_FAIL();
                }
            }
            break;
        case FUNCT3_CSRRWI:
            // CSR Read and Write Immediate
            {
                uint32_t csr = current_inst->imm_i;
                switch (csr)
                {
                case 0x1: // FP Flags
                    x[current_inst->rd] = (fcsr & 0x1F);
                    fcsr &= ~0x1F;
                    fcsr |= (current_inst->rs1 & 0x1F);
                    break;
                case 0x2: // FP default round mode
                    x[current_inst->rd] = (fcsr >> 5) & 0b111;
                    fcsr &= ~(0b111 << 5);
                    fcsr |= ((current_inst->rs1 & 0b111) << 5);
                    break;
                case 0x3: // whole FCSR
                    x[current_inst->rd] = fcsr;
                    fcsr = current_inst->rs1 & 0xFFFFFFFF;
                    break;
                default:
                    // Unknown CSR
                    _trace("RiscVEmulator: Unknown CSR access, instruction: 0x%08x\n", current_inst->inst);
                    result.status = ExecuteResult::Status::IllegalInstruction;
                    result.illegal_instruction.instruction = current_inst->inst;
                    OPCODE_RETURN_FAIL();
                }
            }
            break;
        case FUNCT3_CSRRSI:
            // CSR Read and Set Immediate
            {
                uint32_t csr = current_inst->imm_i;
                switch (csr)
                {
                case 0x1: // FP Flags
                    x[current_inst->rd] = (fcsr & 0x1F);
                    fcsr |= (current_inst->rs1 & 0x1F);
                    break;
                case 0x2: // FP default round mode
                    x[current_inst->rd] = (fcsr >> 5) & 0b111;
                    fcsr |= ((current_inst->rs1 & 0b111) << 5);
                    break;
                case 0x3: // whole FCSR
                    x[current_inst->rd] = fcsr;
                    fcsr |= (current_inst->rs1 & 0xFFFFFFFF);
                    break;
                default:
                    // Unknown CSR
                    _trace("RiscVEmulator: Unknown CSR access, instruction: 0x%08x\n", current_inst->inst);
                    result.status = ExecuteResult::Status::IllegalInstruction;
                    result.illegal_instruction.instruction = current_inst->inst;
                    OPCODE_RETURN_FAIL();
                }
            }
            break;
        case FUNCT3_CSRRCI:
            // CSR Read and Clear Immediate
            {
                uint32_t csr = current_inst->imm_i;
                switch (csr)
                {
                case 0x1: // FP Flags
                    x[current_inst->rd] = (fcsr & 0x1F);
                    fcsr &= ~(current_inst->rs1 & 0x1F);
                    break;
                case 0x2: // FP default round mode
                    x[current_inst->rd] = (fcsr >> 5) & 0b111;
                    fcsr &= ~((current_inst->rs1 & 0b111) << 5);
                    break;
                case 0x3: // whole FCSR
                    x[current_inst->rd] = fcsr;
                    fcsr &= ~(current_inst->rs1 & 0xFFFFFFFF);
                    break;
                default:
                    // Unknown CSR
                    _trace("RiscVEmulator: Unknown CSR access, instruction: 0x%08x\n", current_inst->inst);
                    result.status = ExecuteResult::Status::IllegalInstruction;
                    result.illegal_instruction.instruction = current_inst->inst;
                    OPCODE_RETURN_FAIL();
                }
            }
            break;
        default:
            // Unknown funct3
            _trace("RiscVEmulator: Unknown funct3 for OP_CSR, instruction: 0x%08x\n", current_inst->inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        DISPATCH();
    case_op_misc_mem:
        // FENCE
        if (current_inst->funct3 == FUNCT3_FENCE)
        {
            // No operation
        }
        else if (current_inst->funct3 == FUNCT3_FENCE_I)
        {
            // FENCE.I
            // End the current trace early, to re-fetch instructions next time
            result.status = ExecuteResult::Status::Success;
            OPCODE_RETURN_OK();
        }
        else
        {
            // Unknown funct3
            _trace("RiscVEmulator: Unknown funct3 for OP_MISC_MEM, instruction: 0x%08x\n", current_inst->inst);
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        DISPATCH();
    case_op_atomic:
        if (current_inst->funct3 != 0x2)
        {
            // Unknown funct3
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        switch (extract_funct5(current_inst->inst))
        {
        case FUNCT5_LR:
        {
            // Load Reserved
            uint32_t val;
            if (read32(x[current_inst->rs1], val) != 0)
            {
                _trace("RiscVEmulator: LR: Load from unreadable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            memory->reserved_mem[x[current_inst->rs1]] = reserved_mem_id;
            x[current_inst->rd] = val;
            break;
        }
        case FUNCT5_SC:
        {
            // Store Conditional
            auto it = memory->reserved_mem.find(x[current_inst->rs1]);
            if (it == memory->reserved_mem.end() || it->second != reserved_mem_id)
            {
                // Not reserved
                x[current_inst->rd] = 1;
                break;
            }
            uint32_t val = x[current_inst->rs2];
            if (write32(x[current_inst->rs1], val) != 0)
            {
                _trace("RiscVEmulator: SC: Store to bad address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = val;
                OPCODE_RETURN_FAIL();
            }
            memory->reserved_mem.erase(it);
            x[current_inst->rd] = 0;
            break;
        }
        case FUNCT5_AMOSWAP:
        {
            // Atomic Swap
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                _trace("RiscVEmulator: AMOSWAP: Swap from unreadable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = x[current_inst->rs2];
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                _trace("RiscVEmulator: AMOSWAP: Swap to unwritable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOADD:
        {
            // Atomic Add
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                _trace("RiscVEmulator: AMOADD: Load from unreadable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = old_val + x[current_inst->rs2];
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                _trace("RiscVEmulator: AMOADD: Store to unwritable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOAND:
        {
            // Atomic AND
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                _trace("RiscVEmulator: AMOAND: Load from unreadable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = old_val & x[current_inst->rs2];
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                _trace("RiscVEmulator: AMOAND: Store to unwritable address 0x%08x\n", x[current_inst->rs1]);
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOOR:
        {
            // Atomic OR
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = old_val | x[current_inst->rs2];
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOXOR:
        {
            // Atomic XOR
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = old_val ^ x[current_inst->rs2];
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOMAX:
        {
            // Atomic Max
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = std::max((int32_t)old_val, (int32_t)x[current_inst->rs2]);
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOMIN:
        {
            // Atomic Min
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = std::min((int32_t)old_val, (int32_t)x[current_inst->rs2]);
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOMAXU:
        {
            // Atomic Max Unsigned
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = std::max(old_val, x[current_inst->rs2]);
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        case FUNCT5_AMOMINU:
        {
            // Atomic Min Unsigned
            uint32_t old_val;
            if (read32(x[current_inst->rs1], old_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1];
                OPCODE_RETURN_FAIL();
            }
            uint32_t new_val = std::min(old_val, x[current_inst->rs2]);
            if (write32(x[current_inst->rs1], new_val) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1];
                result.illegal_store.value = new_val;
                OPCODE_RETURN_FAIL();
            }
            x[current_inst->rd] = old_val;
            break;
        }
        default:
            // Unknown funct5
            result.status = ExecuteResult::Status::IllegalInstruction;
            result.illegal_instruction.instruction = current_inst->inst;
            OPCODE_RETURN_FAIL();
        }
        DISPATCH();
    case_op_flw:
        if (current_inst->funct3 & 0x1)
        {
            // FLD
            double value;
            if (readf64(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            f[current_inst->rd] = value;
        }
        else
        {
            // FLW
            float value;
            if (readf32(x[current_inst->rs1] + current_inst->imm_i, value) != 0)
            {
                result.status = ExecuteResult::Status::IllegalLoad;
                result.illegal_load.address = x[current_inst->rs1] + current_inst->imm_i;
                OPCODE_RETURN_FAIL();
            }
            write_float_to_double(value, f[current_inst->rd]);
        }
        DISPATCH();
    case_op_fsw:
        if (current_inst->funct3 & 0x1)
        {
            // FSD
            if (writef64(x[current_inst->rs1] + current_inst->imm_s, f[current_inst->rs2]) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1] + current_inst->imm_s;
                result.illegal_store.value = f[current_inst->rs2];
                OPCODE_RETURN_FAIL();
            }
        }
        else
        {
            // FSW
            float value = read_float_from_double(f[current_inst->rs2]);
            if (writef32(x[current_inst->rs1] + current_inst->imm_s, value) != 0)
            {
                result.status = ExecuteResult::Status::IllegalStore;
                result.illegal_store.address = x[current_inst->rs1] + current_inst->imm_s;
                result.illegal_store.value = value;
                OPCODE_RETURN_FAIL();
            }
        }
        DISPATCH();
    case_op_fmadd:
        // FMADD
        if (is_double_precision(current_inst->inst))
        {
            double a = f[current_inst->rs1];
            double b = f[current_inst->rs2];
            double c = f[extract_rs3(current_inst->inst)];
            f[current_inst->rd] = a * b + c;
        }
        else
        {
            float a = read_float_from_double(f[current_inst->rs1]);
            float b = read_float_from_double(f[current_inst->rs2]);
            float c = read_float_from_double(f[extract_rs3(current_inst->inst)]);
            write_float_to_double(a * b + c, f[current_inst->rd]);
        }
        DISPATCH();
    case_op_fmsub:
        // FMSUB
        if (is_double_precision(current_inst->inst))
        {
            double a = f[current_inst->rs1];
            double b = f[current_inst->rs2];
            double c = f[extract_rs3(current_inst->inst)];
            f[current_inst->rd] = a * b - c;
        }
        else
        {
            float a = read_float_from_double(f[current_inst->rs1]);
            float b = read_float_from_double(f[current_inst->rs2]);
            float c = read_float_from_double(f[extract_rs3(current_inst->inst)]);
            write_float_to_double(a * b - c, f[current_inst->rd]);
        }
        DISPATCH();
    case_op_fnmadd:
        // FNMADD
        if (is_double_precision(current_inst->inst))
        {
            double a = f[current_inst->rs1];
            double b = f[current_inst->rs2];
            double c = f[extract_rs3(current_inst->inst)];
            f[current_inst->rd] = -(a * b) - c;
        }
        else
        {
            float a = read_float_from_double(f[current_inst->rs1]);
            float b = read_float_from_double(f[current_inst->rs2]);
            float c = read_float_from_double(f[extract_rs3(current_inst->inst)]);
            write_float_to_double(-(a * b) - c, f[current_inst->rd]);
        }
        DISPATCH();
    case_op_fnmsub:
        // FNMSUB
        if (is_double_precision(current_inst->inst))
        {
            double a = f[current_inst->rs1];
            double b = f[current_inst->rs2];
            double c = f[extract_rs3(current_inst->inst)];
            f[current_inst->rd] = -(a * b) + c;
        }
        else
        {
            float a = read_float_from_double(f[current_inst->rs1]);
            float b = read_float_from_double(f[current_inst->rs2]);
            float c = read_float_from_double(f[extract_rs3(current_inst->inst)]);
            write_float_to_double(-(a * b) + c, f[current_inst->rd]);
        }
        DISPATCH();
    case_op_freg:
        float a, b;
        double ad, bd;
        switch (current_inst->funct7)
        {
        case 0b0000000:
            // FADD.S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            write_float_to_double(a + b, f[current_inst->rd]);
            break;
        case 0b0000100:
            // FSUB.S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            write_float_to_double(a - b, f[current_inst->rd]);
            break;
        case 0b0001000:
            // FMUL.S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            write_float_to_double(a * b, f[current_inst->rd]);
            break;
        case 0b0001100:
            // FDIV.S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            write_float_to_double(a / b, f[current_inst->rd]);
            break;
        case 0b0101100:
            // FSQRT.S
            a = read_float_from_double(f[current_inst->rs1]);
            write_float_to_double(sqrt(a), f[current_inst->rd]);
            break;
        case 0b0010000:
            // FSGN*.S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            switch (current_inst->funct3)
            {
            case 0b000:
                // FSGNJ.S
                write_float_to_double(b < 0 ? -abs(a) : abs(a),
                                      f[current_inst->rd]);
                break;
            case 0b001:
                // FSGNJN.S
                write_float_to_double(b < 0 ? abs(a) : -abs(a),
                                      f[current_inst->rd]);
                break;
            case 0b010:
                // FSGNJX.S
                write_float_to_double(b < 0 ? -a : a,
                                      f[current_inst->rd]);
                break;
            default:
                // Unknown funct3
                _trace("RiscVEmulator: Unknown funct3 for OP_IMM, instruction: 0x%08x\n", current_inst->inst);
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b0010100:
            // F(MIN|MAX).S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            if (current_inst->funct3 == 0b000)
                // FMIN.S
                write_float_to_double(std::min(a, b), f[current_inst->rd]);
            else if (current_inst->funct3 == 0b001)
                // FMAX.S
                write_float_to_double(std::max(a, b), f[current_inst->rd]);
            else
            {
                // Unknown funct3
                _trace("RiscVEmulator: Unknown funct3 for OP_FREG, instruction: 0x%08x\n", current_inst->inst);
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1100000:
            // FCVT.W*.S
            switch (current_inst->rs2)
            {
            case 0b00000:
                // FCVT.W.S
                a = read_float_from_double(f[current_inst->rs1]);
                if (a > INT32_MAX || a < INT32_MIN)
                {
                    // NV
                    fcsr |= 1 << 4;
                    break;
                }
                x[current_inst->rd] = (int32_t)a;
                break;
            case 0b00001:
                // FCVT.WU.S
                a = read_float_from_double(f[current_inst->rs1]);
                if (a > UINT32_MAX || a < 0)
                {
                    // NV
                    fcsr |= 1 << 4;
                    break;
                }
                x[current_inst->rd] = (uint32_t)a;
                break;
            default:
                // Unknown funct3
                _trace("RiscVEmulator: Unknown funct3 for OP_FREG, instruction: 0x%08x\n", current_inst->inst);
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1110000:
            // FMV.X.W or FCLASS.S
            switch (current_inst->funct3)
            {
            case 0b000:
                // FMV.X.W
                a = read_float_from_double(f[current_inst->rs1]);
                memcpy(&x[current_inst->rd], &a, sizeof(float));
                break;
            case 0b001:
                // FCLASS.S
                a = read_float_from_double(f[current_inst->rs1]);
                x[current_inst->rd] = classify_float(a);
                break;
            default:
                // Unknown funct3
                _trace("RiscVEmulator: Unknown funct3 for OP_FREG, instruction: 0x%08x\n", current_inst->inst);
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1010000:
            // FEQ.S or FLT.S or FLE.S
            a = read_float_from_double(f[current_inst->rs1]);
            b = read_float_from_double(f[current_inst->rs2]);
            switch (current_inst->funct3)
            {
            case 0b000:
                // FEQ.S
                x[current_inst->rd] = (a == b);
                break;
            case 0b001:
                // FLT.S
                x[current_inst->rd] = (a < b);
                break;
            case 0b010:
                // FLE.S
                x[current_inst->rd] = (a <= b);
                break;
            default:
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1101000:
            // FCVT.S.W*
            switch (current_inst->rs2)
            {
            case 0b00000:
                // FCVT.S.W
                a = (float)(int32_t)x[current_inst->rs1];
                write_float_to_double(a, f[current_inst->rd]);
                break;
            case 0b00001:
                // FCVT.S.WU
                a = (float)(uint32_t)x[current_inst->rs1];
                write_float_to_double(a, f[current_inst->rd]);
                break;
            default:
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1111000:
            // FMV.W.X
            memcpy(&a, &x[current_inst->rs1], sizeof(float));
            write_float_to_double(a, f[current_inst->rd]);
            break;
        case 0b0000001:
            // FADD.D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            f[current_inst->rd] = ad + bd;
            break;
        case 0b0000101:
            // FSUB.D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            f[current_inst->rd] = ad - bd;
            break;
        case 0b0001001:
            // FMUL.D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            f[current_inst->rd] = ad * bd;
            break;
        case 0b0001101:
            // FDIV.D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            f[current_inst->rd] = ad / bd;
            break;
        case 0b0101101:
            // FSQRT.D
            ad = f[current_inst->rs1];
            f[current_inst->rd] = sqrt(ad);
            break;
        case 0b0010001:
            // FSGN*.D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            switch (current_inst->funct3)
            {
            case 0b000:
                // FSGNJ.D
                f[current_inst->rd] = bd < 0 ? -abs(ad) : abs(ad);
                break;
            case 0b001:
                // FSGNJN.D
                f[current_inst->rd] = bd < 0 ? abs(ad) : -abs(ad);
                break;
            case 0b010:
                // FSGNJX.D
                f[current_inst->rd] = bd < 0 ? -ad : ad;
                break;
            default:
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b0010101:
            // F(MIN|MAX).D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            if (current_inst->funct3 == 0b000)
                // FMIN.D
                f[current_inst->rd] = std::min(ad, bd);
            else if (current_inst->funct3 == 0b001)
                // FMAX.D
                f[current_inst->rd] = std::max(ad, bd);
            else
            {
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b0100000:
            // FCVT.S.D
            ad = f[current_inst->rs1];
            write_float_to_double((float)ad, f[current_inst->rd]);
            break;
        case 0b0100001:
            // FCVT.D.S
            ad = read_float_from_double(f[current_inst->rs1]);
            f[current_inst->rd] = ad;
            break;
        case 0b1010001:
            // FEQ.D or FLT.D or FLE.D
            ad = f[current_inst->rs1];
            bd = f[current_inst->rs2];
            switch (current_inst->funct3)
            {
            case 0b000:
                // FEQ.D
                x[current_inst->rd] = (ad == bd);
                break;
            case 0b001:
                // FLT.D
                x[current_inst->rd] = (ad < bd);
                break;
            case 0b010:
                // FLE.D
                x[current_inst->rd] = (ad <= bd);
                break;
            default:
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1110001:
            // FCLASS.D
            ad = f[current_inst->rs1];
            x[current_inst->rd] = classify_double(ad);
            break;
        case 0b1100001:
            // FCVT.W.D or FCVT.WU.D
            ad = f[current_inst->rs1];
            switch (current_inst->rs2)
            {
            case 0b00000:
                // FCVT.W.D
                if (ad > INT32_MAX || ad < INT32_MIN)
                {
                    // NV
                    fcsr |= 1 << 4;
                    break;
                }
                x[current_inst->rd] = (int32_t)std::nearbyint(ad);
                break;
            case 0b00001:
                // FCVT.WU.D
                if (ad > UINT32_MAX || ad < 0)
                {
                    // NV
                    fcsr |= 1 << 4;
                    break;
                }
                x[current_inst->rd] = (uint32_t)std::nearbyint(ad);
                break;
            default:
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
            break;
        case 0b1101001:
            // FCVT.D.W*
            switch (current_inst->rs2)
            {
            case 0b00000:
                // FCVT.D.W
                ad = (double)(int32_t)x[current_inst->rs1];
                f[current_inst->rd] = ad;
                break;
            case 0b00001:
                // FCVT.D.WU
                ad = (double)(uint32_t)x[current_inst->rs1];
                f[current_inst->rd] = ad;
                break;
            default:
                // Unknown funct3
                result.status = ExecuteResult::Status::IllegalInstruction;
                result.illegal_instruction.instruction = current_inst->inst;
                OPCODE_RETURN_FAIL();
            }
        }
        DISPATCH();

    case_invalid_op:
        result.status = ExecuteResult::Status::IllegalInstruction;
        result.illegal_instruction.instruction = current_inst->inst;
        OPCODE_RETURN_FAIL();
    }
} // namespace Hamster

// ignored -Wpedantic
#pragma GCC diagnostic pop
