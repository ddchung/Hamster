
#include <process/riscv_rv32i_thread.hpp>
#include <syscall/syscall.hpp>
#include <cstring>

namespace Hamster
{
    namespace
    {
        uint32_t sign_extend(uint32_t value, uint32_t bits)
        {
            if (value & (1 << (bits - 1)))
            {
                value |= ~((1 << bits) - 1);
            }
            return value;
        }

        // Extraction

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

        uint32_t extract_funct7(uint32_t inst)
        {
            return (inst >> 25) & 0x7F;
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
                ((inst >> 31) & 0x1) << 12 |      // imm[12]
                    ((inst >> 7) & 0x1) << 11 |   // imm[11]
                    ((inst >> 25) & 0x3F) << 5 | // imm[10:5]
                    ((inst >> 8) & 0xF) << 1,     // imm[4:1]
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

        enum Opcode
        {
            LOAD = 0b0000011,
            LOAD_FP = 0b0000111,
            MISC_MEM = 0b0001111,
            OP_IMM = 0b0010011,
            AUIPC = 0b0010111,
            OP_IMM_32 = 0b0011011,
            STORE = 0b0100011,
            STORE_FP = 0b0100111,
            AMO = 0b0101111,
            OP = 0b0110011,
            LUI = 0b0110111,
            OP_32 = 0b0111011,
            MADD = 0b1000011,
            MSUB = 0b1000111,
            NMSUB = 0b1001011,
            NMADD = 0b1001111,
            OP_FP = 0b1010011,
            BRANCH = 0b1100011,
            JALR = 0b1100111,
            JAL = 0b1101111,
            SYSTEM = 0b1110011
        };

        const char *reg_names[] = {
            "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
            "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
            "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
            "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
        };
    } // namespace

    RV32Thread::RV32Thread(MemorySpace &mem_sp)
        : BaseThread(mem_sp), pc(0)
    {
        memset(regs, 0, sizeof(regs));

        // Set up stack
        uint32_t stack_top = 0x80000000;
        uint32_t stack_size = 0x4000; // 16KB stack
        uint32_t stack_bottom = stack_top - stack_size;

        // Allocate stack space
        for (uint32_t addr = MemorySpace::get_page_start(stack_bottom); addr <= MemorySpace::get_page_start(stack_top + 0x1000); addr += HAMSTER_PAGE_SIZE)
        {
            mem.allocate_page(addr);
        }

        write32(stack_top, 0);
        write32(stack_top - 4, 0);

        // Initialize stack pointer
        regs[2] = stack_top - 4; // x2 is sp

        // Initialize frame pointer
        regs[8] = stack_top - 4; // x8 is fp

        // Initialize return address
        regs[1] = 0xffffffff; // x1 is ra
    }

    int RV32Thread::set_start_addr(uint64_t addr)
    {
        pc = addr;
        return 0;
    }

    int RV32Thread::tick()
    {
        if (!mem.is_page_range_allocated(pc, 4))
        {
            return -1; // Page fault
        }

        uint32_t inst = read32(pc);

        uint32_t old_pc = pc;
        int ret = execute(inst);

        if (pc == old_pc)
            pc += 4; // If the instruction didn't change the PC, increment it by 4

        return ret;
    }

    Syscall RV32Thread::get_syscall()
    {
        Syscall sys;
        sys.syscall_num = regs[17];
        sys.arg1 = regs[10];
        sys.arg2 = regs[11];
        sys.arg3 = regs[12];
        sys.arg4 = regs[13];
        sys.arg5 = regs[14];
        sys.arg6 = regs[15];
        return sys;
    }

    void RV32Thread::set_syscall_ret(uint64_t ret)
    {
        regs[10] = ret; // Return value in a0
    }

    uint32_t RV32Thread::read32(uint32_t addr)
    {
        uint32_t value = 0;
        mem.memcpy((uint8_t *)&value, addr, sizeof(value));
        return value;
    }

    uint16_t RV32Thread::read16(uint32_t addr)
    {
        uint16_t value = 0;
        mem.memcpy((uint8_t *)&value, addr, sizeof(value));
        return value;
    }

    uint8_t RV32Thread::read8(uint32_t addr)
    {
        uint8_t value = 0;
        mem.memcpy((uint8_t *)&value, addr, sizeof(value));
        return value;
    }

    void RV32Thread::write32(uint32_t addr, uint32_t value)
    {
        if (!mem.is_page_range_allocated(addr, sizeof(value)))
            mem.allocate_page(addr);
        mem.memcpy(addr, (uint8_t *)&value, sizeof(value));
    }

    void RV32Thread::write16(uint32_t addr, uint16_t value)
    {
        if (!mem.is_page_range_allocated(addr, sizeof(value)))
            mem.allocate_page(addr);
        mem.memcpy(addr, (uint8_t *)&value, sizeof(value));
    }

    void RV32Thread::write8(uint32_t addr, uint8_t value)
    {
        if (!mem.is_page_range_allocated(addr, sizeof(value)))
            mem.allocate_page(addr);
        mem.memcpy(addr, (uint8_t *)&value, sizeof(value));
    }

    int RV32Thread::execute(uint32_t inst)
    {
        regs[0] = 0; // x0 is always 0

        auto opcode = extract_opcode(inst);
        auto rd = extract_rd(inst);
        auto funct3 = extract_funct3(inst);
        auto rs1 = extract_rs1(inst);
        auto rs2 = extract_rs2(inst);
        auto funct7 = extract_funct7(inst);

        switch (opcode)
        {
        case LUI: // Load Upper Immediate
        {
            uint32_t imm = extract_imm_u(inst);
            regs[rd] = imm;
            return 0;
        }
        case AUIPC: // Add Upper Immediate to PC
        {
            uint32_t imm = extract_imm_u(inst);
            regs[rd] = pc + imm;
            return 0;
        }
        case JAL: // Jump and Link
        {
            int32_t imm = extract_imm_j(inst);
            regs[rd] = pc + 4;
            pc += imm;
            return 0;
        }
        case JALR: // Jump and Link Register
        {
            uint32_t imm = extract_imm_i(inst);
            pc = (regs[rs1] + imm) & ~0x1;
            regs[rd] = pc + 4;
            return 0;
        }
        case BRANCH: // Branch
        {
            uint32_t imm = extract_imm_b(inst);
            switch (funct3)
            {
                case 0b000: // BEQ
                    if (regs[rs1] == regs[rs2])
                        pc += imm;
                    break;
                case 0b001: // BNE
                    if (regs[rs1] != regs[rs2])
                        pc += imm;
                    break;
                case 0b100: // BLT
                    if ((int32_t)regs[rs1] < (int32_t)regs[rs2])
                        pc += imm;
                    break;
                case 0b101: // BGE
                    if ((int32_t)regs[rs1] >= (int32_t)regs[rs2])
                        pc += imm;
                    break;
                case 0b110: // BLTU
                    if (regs[rs1] < regs[rs2])
                        pc += imm;
                    break;
                case 0b111: // BGEU
                    if (regs[rs1] >= regs[rs2])
                        pc += imm;
                    break;
                default:
                    // invalid funct3
                    return -1;
            }
            return 0;
        }
        case LOAD: // Load
        {
            int32_t imm = extract_imm_i(inst);
            uint32_t addr = regs[rs1] + imm;
            switch (funct3)
            {
            case 0x0: // LB
                regs[rd] = sign_extend(read8(addr), 8);
                break;
            case 0x1: // LH
                regs[rd] = sign_extend(read16(addr), 16);
                break;
            case 0x2: // LW
                regs[rd] = read32(addr);
                break;
            case 0x3: // LBU
                regs[rd] = read8(addr);
                break;
            case 0x4: // LHU
                regs[rd] = read16(addr);
                break;
            default:
                return -1; // Invalid funct3 for LOAD
            }
            return 0;
        }
        case STORE: // Store
        {
            int32_t imm = extract_imm_s(inst);
            uint32_t addr = regs[rs1] + imm;
            switch (funct3)
            {
            case 0x0: // SB
                write8(addr, regs[rs2]);
                break;
            case 0x1: // SH
                write16(addr, regs[rs2]);
                break;
            case 0x2: // SW
                write32(addr, regs[rs2]);
                break;
            default:
                return -1; // Invalid funct3 for STORE
            }
            return 0;
        }
        case OP_IMM: // Immediate Arithmetic
        {
            int32_t imm = extract_imm_i(inst);

            switch(funct3)
            {
            case 0b000: // ADDI
                regs[rd] = regs[rs1] + imm;
                break;
            case 0b010: // SLTI
                regs[rd] = (int32_t)regs[rs1] < (int32_t)imm ? 1 : 0;
                break;
            case 0b011: // SLTIU
                regs[rd] = (uint32_t)regs[rs1] < (uint32_t)imm ? 1 : 0;
                break;
            case 0b100: // XORI
                regs[rd] = regs[rs1] ^ imm;
                break;
            case 0b110: // ORI
                regs[rd] = regs[rs1] | imm;
                break;
            case 0b111: // ANDI
                regs[rd] = regs[rs1] & imm;
                break;
            case 0b001: // SLLI
                regs[rd] = regs[rs1] << (imm & 0x1F);
                break;
            case 0b101: // SRLI/SRAI
                if (funct7 & 0x20) // SRAI
                {
                    regs[rd] = (int32_t)regs[rs1] >> (imm & 0x1F);
                }
                else // SRLI
                {
                    regs[rd] = regs[rs1] >> (imm & 0x1F);
                }
                break;
            }
            return 0;
        }
        case OP: // Register Arithmetic
        {
            switch (funct3)
            {
            case 0b000: // ADD/SUB
                if (funct7 & 0x20) // SUB
                {
                    regs[rd] = regs[rs1] - regs[rs2];
                }
                else // ADD
                {
                    regs[rd] = regs[rs1] + regs[rs2];
                }
                break;
            case 0b001: // SLL
                regs[rd] = regs[rs1] << (regs[rs2] & 0x1F);
                break;
            case 0b010: // SLT
                regs[rd] = (int32_t)regs[rs1] < (int32_t)regs[rs2] ? 1 : 0;
                break;
            case 0b011: // SLTU
                regs[rd] = regs[rs1] < regs[rs2] ? 1 : 0;
                break;
            case 0b100: // XOR
                regs[rd] = regs[rs1] ^ regs[rs2];
                break;
            case 0b101: // SRL/SRA
                if (funct7 & 0x20) // SRA
                {
                    regs[rd] = (int32_t)regs[rs1] >> (regs[rs2] & 0x1F);
                }
                else // SRL
                {
                    regs[rd] = regs[rs1] >> (regs[rs2] & 0x1F);
                }
                break;
            case 0b110: // OR
                regs[rd] = regs[rs1] | regs[rs2];
                break;
            case 0b111: // AND
                regs[rd] = regs[rs1] & regs[rs2];
                break;
            }
            return 0;
        }
        case MISC_MEM:
        {
            // NOP in simple implementation
            return 0;
        }
        case SYSTEM:
        {
            return 2;
        }
        default:
        {
            return -1; // Unknown opcode
        }
        }
    }
} // namespace Hamster
