
#include <riscv/riscv_emulator.hpp>
#include <memory/memory_space.hpp>
#include <cassert>
#include <cstring>
#include <cmath>

using namespace Hamster;

void test_emulator()
{
    RiscVEmulator emu;
    MemorySpace mem;
    emu.memory = &mem;

    // Map memory for code
    mem.map_anonymous(0x1000, 0x1000, PERM_READ | PERM_WRITE | PERM_EXEC);

    // Helper lambda to reset emulator state
    auto reset_emu = [&]()
    {
        memset(emu.x, 0, sizeof(emu.x));
        memset(emu.f, 0, sizeof(emu.f));
        emu.fcsr = 0;
        emu.pc = 0x1000;
        emu.flush_caches();
    };

    // Helper lambda to setup and run test
    auto run_test = [&](const char *test_name, void *code, size_t code_size)
    {
        reset_emu();

        // Copy code
        mem.memset(0x1000, 0, HAMSTER_PAGE_SIZE);
        mem.memcpy(0x1000, code, code_size);

        // Run until ECALL
        auto result = emu.run();
        assert(result.status == RiscVEmulator::ExecuteResult::Status::ECALL);
    };

    // TEST 1: Basic Arithmetic Instructions (ADD, SUB, ADDI)
    {
        unsigned char code[32] =
            {
                0x93,
                0x00,
                0xa0,
                0x02,
                0x13,
                0x01,
                0x10,
                0x01,
                0xb3,
                0x81,
                0x20,
                0x00,
                0x33,
                0x82,
                0x20,
                0x40,
                0x93,
                0x82,
                0x60,
                0xff,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 42      # x1 = 42
                addi x2, x0, 17      # x2 = 17
                add x3, x1, x2       # x3 = x1 + x2 = 59
                sub x4, x1, x2       # x4 = x1 - x2 = 25
                addi x5, x1, -10     # x5 = x1 - 10 = 32
                ecall

                */
        run_test("Basic Arithmetic", code, sizeof(code));
        assert(emu.x[1] == 42);
        assert(emu.x[2] == 17);
        assert(emu.x[3] == 59);
        assert(emu.x[4] == 25);
        assert(emu.x[5] == 32);
    }

    // TEST 2: Logical Instructions (AND, OR, XOR, ANDI, ORI, XORI)
    {
        unsigned char code[36] =
            {
                0x93,
                0x00,
                0xf0,
                0x0f,
                0x13,
                0x01,
                0x00,
                0x0f,
                0xb3,
                0xf1,
                0x20,
                0x00,
                0x33,
                0xe2,
                0x20,
                0x00,
                0xb3,
                0xc2,
                0x20,
                0x00,
                0x13,
                0xf3,
                0xf0,
                0x00,
                0x93,
                0x63,
                0xb0,
                0x0a,
                0x13,
                0xc4,
                0xf0,
                0x0f,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 0xFF    # x1 = 0xFF
                addi x2, x0, 0xF0    # x2 = 0xF0
                and x3, x1, x2       # x3 = 0xF0
                or x4, x1, x2        # x4 = 0xFF
                xor x5, x1, x2       # x5 = 0x0F
                andi x6, x1, 0x0F    # x6 = 0x0F
                ori x7, x0, 0xAB     # x7 = 0xAB
                xori x8, x1, 0xFF    # x8 = 0
                ecall

                */
        run_test("Logical Operations", code, sizeof(code));
        assert(emu.x[1] == 0xFF);
        assert(emu.x[2] == 0xF0);
        assert(emu.x[3] == 0xF0);
        assert(emu.x[4] == 0xFF);
        assert(emu.x[5] == 0x0F);
        assert(emu.x[6] == 0x0F);
        assert(emu.x[7] == 0xAB);
        assert(emu.x[8] == 0);
    }

    // TEST 3: Shift Instructions (SLL, SRL, SRA, SLLI, SRLI, SRAI)
    {
        unsigned char code[40] =
            {
                0x93,
                0x00,
                0x80,
                0x00,
                0x13,
                0x01,
                0x20,
                0x00,
                0xb3,
                0x91,
                0x20,
                0x00,
                0x33,
                0xd2,
                0x20,
                0x00,
                0x93,
                0x92,
                0x30,
                0x00,
                0x13,
                0xd3,
                0x10,
                0x00,
                0x93,
                0x03,
                0x00,
                0xff,
                0x13,
                0xd4,
                0x23,
                0x40,
                0x93,
                0xd4,
                0x23,
                0x00,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 8       # x1 = 8
                addi x2, x0, 2       # x2 = 2
                sll x3, x1, x2       # x3 = 8 << 2 = 32
                srl x4, x1, x2       # x4 = 8 >> 2 = 2
                slli x5, x1, 3       # x5 = 8 << 3 = 64
                srli x6, x1, 1       # x6 = 8 >> 1 = 4
                addi x7, x0, -16     # x7 = -16 (0xFFFFFFF0)
                srai x8, x7, 2       # x8 = -16 >> 2 = -4 (arithmetic)
                srli x9, x7, 2       # x9 = 0xFFFFFFF0 >> 2 = 0x3FFFFFFC (logical)
                ecall

                */
        run_test("Shift Operations", code, sizeof(code));
        assert(emu.x[1] == 8);
        assert(emu.x[2] == 2);
        assert(emu.x[3] == 32);
        assert(emu.x[4] == 2);
        assert(emu.x[5] == 64);
        assert(emu.x[6] == 4);
        assert(emu.x[7] == (uint32_t)-16);
        assert(emu.x[8] == (uint32_t)-4);
        assert(emu.x[9] == 0x3FFFFFFC);
    }

    // TEST 4: Comparison Instructions (SLT, SLTU, SLTI, SLTIU)
    {
        unsigned char code[40] =
            {
                0x93,
                0x00,
                0x50,
                0x00,
                0x13,
                0x01,
                0xa0,
                0x00,
                0x93,
                0x01,
                0xf0,
                0xff,
                0x33,
                0xa2,
                0x20,
                0x00,
                0xb3,
                0x22,
                0x11,
                0x00,
                0x33,
                0xa3,
                0x11,
                0x00,
                0xb3,
                0xb3,
                0x11,
                0x00,
                0x13,
                0xa4,
                0xa0,
                0x00,
                0x93,
                0xb4,
                0xa1,
                0x00,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 5       # x1 = 5
                addi x2, x0, 10      # x2 = 10
                addi x3, x0, -1      # x3 = -1 (0xFFFFFFFF)
                slt x4, x1, x2       # x4 = 1 (5 < 10)
                slt x5, x2, x1       # x5 = 0 (10 >= 5)
                slt x6, x3, x1       # x6 = 1 (-1 < 5 signed)
                sltu x7, x3, x1      # x7 = 0 (0xFFFFFFFF >= 5 unsigned)
                slti x8, x1, 10      # x8 = 1 (5 < 10)
                sltiu x9, x3, 10     # x9 = 0 (0xFFFFFFFF >= 10 unsigned)
                ecall

                */
        run_test("Comparison Operations", code, sizeof(code));
        assert(emu.x[4] == 1);
        assert(emu.x[5] == 0);
        assert(emu.x[6] == 1);
        assert(emu.x[7] == 0);
        assert(emu.x[8] == 1);
        assert(emu.x[9] == 0);
    }

    // TEST 5: Load Upper Immediate (LUI, AUIPC)
    {
        unsigned char code[16] =
            {
                0xb7,
                0x50,
                0x34,
                0x12,
                0x17,
                0x01,
                0x00,
                0x00,
                0x97,
                0x11,
                0x00,
                0x00,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                lui x1, 0x12345      # x1 = 0x12345000
                auipc x2, 0          # x2 = pc
                auipc x3, 1          # x3 = pc + 0x1000
                ecall

                */
        run_test("Load Upper Immediate", code, sizeof(code));
        assert(emu.x[1] == 0x12345000);
        assert(emu.x[2] == 0x1004); // PC after auipc
        assert(emu.x[3] == 0x2008); // PC + 0x1000 after second auipc
    }

    // TEST 6: Load/Store Instructions (LW, LH, LB, SW, SH, SB, LBU, LHU)
    {
        unsigned char code[76] =
            {
                0x93,
                0x00,
                0x00,
                0x50,
                0x13,
                0x01,
                0x20,
                0x01,
                0x93,
                0x01,
                0x40,
                0x03,
                0x13,
                0x02,
                0x60,
                0x05,
                0x93,
                0x02,
                0x80,
                0x07,
                0x23,
                0x80,
                0x20,
                0x00,
                0xa3,
                0x80,
                0x30,
                0x00,
                0x23,
                0x81,
                0x40,
                0x00,
                0xa3,
                0x81,
                0x50,
                0x00,
                0x03,
                0xa3,
                0x00,
                0x00,
                0x83,
                0x93,
                0x00,
                0x00,
                0x03,
                0xd4,
                0x00,
                0x00,
                0x83,
                0x84,
                0x30,
                0x00,
                0x03,
                0xc5,
                0x30,
                0x00,
                0x93,
                0x05,
                0xf0,
                0xff,
                0x23,
                0x82,
                0xb0,
                0x00,
                0x03,
                0x86,
                0x40,
                0x00,
                0x83,
                0xc6,
                0x40,
                0x00,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 0x500   # x1 = 0x500 (data address)
                addi x2, x0, 0x12    # x2 = 0x12
                addi x3, x0, 0x34    # x3 = 0x34
                addi x4, x0, 0x56    # x4 = 0x56
                addi x5, x0, 0x78    # x5 = 0x78
                sb x2, 0(x1)         # Store byte
                sb x3, 1(x1)         # Store byte
                sb x4, 2(x1)         # Store byte
                sb x5, 3(x1)         # Store byte
                lw x6, 0(x1)         # Load word: x6 = 0x78563412
                lh x7, 0(x1)         # Load halfword signed: x7 = 0x3412
                lhu x8, 0(x1)        # Load halfword unsigned: x8 = 0x3412
                lb x9, 3(x1)         # Load byte signed: x9 = 0x78
                lbu x10, 3(x1)       # Load byte unsigned: x10 = 0x78
                addi x11, x0, -1     # x11 = 0xFF
                sb x11, 4(x1)        # Store 0xFF
                lb x12, 4(x1)        # Load byte signed: x12 = -1 (0xFFFFFFFF)
                lbu x13, 4(x1)       # Load byte unsigned: x13 = 0xFF
                ecall
                */
        // Setup data memory
        mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        run_test("Load/Store Operations", code, sizeof(code));
        assert(emu.x[6] == 0x78563412);
        assert(emu.x[7] == 0x3412);
        assert(emu.x[8] == 0x3412);
        assert(emu.x[9] == 0x78);
        assert(emu.x[10] == 0x78);
        assert(emu.x[12] == (uint32_t)-1);
        assert(emu.x[13] == 0xFF);
    }

    // TEST 7: Branch Instructions (BEQ, BNE, BLT, BGE, BLTU, BGEU)
    {
        unsigned char code[72] =
            {
                0x93,
                0x00,
                0xa0,
                0x00,
                0x13,
                0x01,
                0xa0,
                0x00,
                0x93,
                0x01,
                0x50,
                0x00,
                0x13,
                0x05,
                0x00,
                0x00,
                0x63,
                0x84,
                0x20,
                0x00,
                0x13,
                0x05,
                0x15,
                0x00,
                0x63,
                0x94,
                0x30,
                0x00,
                0x13,
                0x05,
                0x25,
                0x00,
                0x63,
                0xc4,
                0x11,
                0x00,
                0x13,
                0x05,
                0x45,
                0x00,
                0x63,
                0xd4,
                0x30,
                0x00,
                0x13,
                0x05,
                0x85,
                0x00,
                0x13,
                0x02,
                0xf0,
                0xff,
                0x63,
                0xe4,
                0x40,
                0x00,
                0x13,
                0x05,
                0x05,
                0x01,
                0x63,
                0x74,
                0x12,
                0x00,
                0x13,
                0x05,
                0x05,
                0x02,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 10      # x1 = 10
                addi x2, x0, 10      # x2 = 10
                addi x3, x0, 5       # x3 = 5
                addi x10, x0, 0      # x10 = 0 (result register)

                beq x1, x2, skip1    # Should branch (10 == 10)
                addi x10, x10, 1     # Should skip
                skip1:

                bne x1, x3, skip2    # Should branch (10 != 5)
                addi x10, x10, 2     # Should skip
                skip2:

                blt x3, x1, skip3    # Should branch (5 < 10)
                addi x10, x10, 4     # Should skip
                skip3:

                bge x1, x3, skip4    # Should branch (10 >= 5)
                addi x10, x10, 8     # Should skip
                skip4:

                addi x4, x0, -1      # x4 = -1 (0xFFFFFFFF)
                bltu x1, x4, skip5   # Should branch (10 < 0xFFFFFFFF unsigned)
                addi x10, x10, 16    # Should skip
                skip5:

                bgeu x4, x1, skip6   # Should branch (0xFFFFFFFF >= 10 unsigned)
                addi x10, x10, 32    # Should skip
                skip6:

                ecall

                */
        run_test("Branch Operations", code, sizeof(code));
        assert(emu.x[10] == 0); // All branches taken, no increments
    }

    // TEST 8: Jump Instructions (JAL, JALR)
    {
        unsigned char code[44] =
            {
                0x13,
                0x05,
                0x00,
                0x00,
                0xef,
                0x00,
                0xc0,
                0x00,
                0x13,
                0x05,
                0x15,
                0x00,
                0x13,
                0x05,
                0x25,
                0x00,
                0x13,
                0x05,
                0x45,
                0x00,
                0x17,
                0x01,
                0x00,
                0x00,
                0x13,
                0x01,
                0x01,
                0x01,
                0xe7,
                0x01,
                0x01,
                0x00,
                0x13,
                0x05,
                0x85,
                0x00,
                0x13,
                0x05,
                0x05,
                0x01,
                0x73,
                0x00,
                0x00,
                0x00,
            };

        /*
                addi x10, x0, 0      # x10 = 0
                jal x1, target1      # Jump to target1, x1 = return address
                addi x10, x10, 1     # Should skip
                addi x10, x10, 2     # Should skip
                target1:
                addi x10, x10, 4     # x10 = 4
                la x2, target2       # x2 = address of target2
                jalr x3, x2, 0       # Jump to x2 + 0, x3 = return address
                addi x10, x10, 8     # Should skip
                # (32 bytes from start = target2)
                target2:
                addi x10, x10, 16    # x10 = 20
                ecall

                */
        run_test("Jump Operations", code, sizeof(code));
        assert(emu.x[10] == 20);
        assert(emu.x[1] != 0); // Return address stored
        assert(emu.x[3] != 0); // Return address stored
    }

    // TEST 9: Multiply Extension (MUL, MULH, MULHSU, MULHU)
    {
        unsigned char code[36] =
            {
                0x93,
                0x00,
                0xa0,
                0x00,
                0x13,
                0x01,
                0x40,
                0x01,
                0xb3,
                0x81,
                0x20,
                0x02,
                0x13,
                0x02,
                0xb0,
                0xff,
                0xb3,
                0x82,
                0x40,
                0x02,
                0x37,
                0x03,
                0x00,
                0x10,
                0xb3,
                0x03,
                0x63,
                0x02,
                0x33,
                0x14,
                0x63,
                0x02,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 10      # x1 = 10
                addi x2, x0, 20      # x2 = 20
                mul x3, x1, x2       # x3 = 200
                addi x4, x0, -5      # x4 = -5
                mul x5, x1, x4       # x5 = -50
                lui x6, 0x10000      # x6 = 0x10000000
                mul x7, x6, x6       # x7 = overflow (lower 32 bits)
                mulh x8, x6, x6      # x8 = upper 32 bits of multiplication
                ecall

                */
        run_test("Multiply Operations", code, sizeof(code));
        assert(emu.x[3] == 200);
        assert(emu.x[5] == (uint32_t)-50);
    }

    // TEST 10: Divide Extension (DIV, DIVU, REM, REMU)
    {
        unsigned char code[40] =
            {
                0x93,
                0x00,
                0x40,
                0x06,
                0x13,
                0x01,
                0x70,
                0x00,
                0xb3,
                0xc1,
                0x20,
                0x02,
                0x33,
                0xe2,
                0x20,
                0x02,
                0x93,
                0x02,
                0xc0,
                0xf9,
                0x33,
                0xc3,
                0x22,
                0x02,
                0xb3,
                0xe3,
                0x22,
                0x02,
                0x33,
                0xd4,
                0x22,
                0x02,
                0xb3,
                0xf4,
                0x22,
                0x02,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, 100     # x1 = 100
                addi x2, x0, 7       # x2 = 7
                div x3, x1, x2       # x3 = 100 / 7 = 14
                rem x4, x1, x2       # x4 = 100 % 7 = 2
                addi x5, x0, -100    # x5 = -100
                div x6, x5, x2       # x6 = -100 / 7 = -14
                rem x7, x5, x2       # x7 = -100 % 7 = -2
                divu x8, x5, x2      # x8 = 0xFFFFFF9C / 7 (unsigned)
                remu x9, x5, x2      # x9 = 0xFFFFFF9C % 7 (unsigned)
                ecall

                */
        run_test("Divide Operations", code, sizeof(code));
        assert(emu.x[3] == 14);
        assert(emu.x[4] == 2);
        assert(emu.x[6] == (uint32_t)-14);
        assert(emu.x[7] == (uint32_t)-2);
    }

    // TEST 11: Edge Cases - Zero Register
    {
        unsigned char code[24] =
            {
                0x13,
                0x00,
                0x40,
                0x06,
                0xb3,
                0x00,
                0x00,
                0x00,
                0x13,
                0x01,
                0x50,
                0x00,
                0x33,
                0x00,
                0x21,
                0x00,
                0xb3,
                0x01,
                0x20,
                0x00,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x0, x0, 100     # Should have no effect
                add x1, x0, x0       # x1 = 0
                addi x2, x0, 5       # x2 = 5
                add x0, x2, x2       # Should have no effect on x0
                add x3, x0, x2       # x3 = 5
                ecall

                */
        run_test("Zero Register Immutability", code, sizeof(code));
        assert(emu.x[0] == 0);
        assert(emu.x[1] == 0);
        assert(emu.x[2] == 5);
        assert(emu.x[3] == 5);
    }

    // TEST 12: Sign Extension Tests
    {
        unsigned char code[28] =
            {
                0x93,
                0x00,
                0xf0,
                0xff,
                0x13,
                0x01,
                0xf0,
                0x7f,
                0x93,
                0x01,
                0x00,
                0x80,
                0x13,
                0xf2,
                0xf0,
                0xff,
                0x93,
                0x62,
                0xf0,
                0xff,
                0x13,
                0x43,
                0xf0,
                0xff,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                addi x1, x0, -1      # x1 = 0xFFFFFFFF
                addi x2, x0, 0x7FF   # x2 = 0x7FF (max positive 12-bit immediate)
                addi x3, x0, -0x800  # x3 = 0xFFFFF800 (min negative 12-bit immediate)
                andi x4, x1, -1      # x4 = 0xFFFFFFFF & 0xFFFFFFFF = 0xFFFFFFFF
                ori x5, x0, -1       # x5 = 0 | 0xFFFFFFFF = 0xFFFFFFFF
                xori x6, x0, -1      # x6 = 0 ^ 0xFFFFFFFF = 0xFFFFFFFF
                ecall

                */
        run_test("Sign Extension", code, sizeof(code));
        assert(emu.x[1] == 0xFFFFFFFF);
        assert(emu.x[2] == 0x7FF);
        assert(emu.x[3] == 0xFFFFF800);
        assert(emu.x[4] == 0xFFFFFFFF);
        assert(emu.x[5] == 0xFFFFFFFF);
        assert(emu.x[6] == 0xFFFFFFFF);
    }

    // TEST 13: Complex Sequence - Fibonacci
    {
        unsigned char code[40] =
            {
                0x93,
                0x00,
                0x00,
                0x00,
                0x13,
                0x01,
                0x10,
                0x00,
                0x13,
                0x05,
                0xa0,
                0x00,
                0x63,
                0x0c,
                0x05,
                0x00,
                0xb3,
                0x81,
                0x20,
                0x00,
                0x93,
                0x00,
                0x01,
                0x00,
                0x13,
                0x81,
                0x01,
                0x00,
                0x13,
                0x05,
                0xf5,
                0xff,
                0x6f,
                0xf0,
                0xdf,
                0xfe,
                0x73,
                0x00,
                0x00,
                0x00,
            };
        /*
                # Calculate 10th Fibonacci number
                addi x1, x0, 0       # x1 = fib(0) = 0
                addi x2, x0, 1       # x2 = fib(1) = 1
                addi x10, x0, 10     # x10 = counter
                loop:
                beq x10, x0, done    # if counter == 0, done
                add x3, x1, x2       # x3 = fib(n)
                addi x1, x2, 0       # x1 = fib(n-1)
                addi x2, x3, 0       # x2 = fib(n)
                addi x10, x10, -1    # counter--
                jal x0, loop         # loop
                done:
                ecall

                */
        run_test("Fibonacci Sequence", code, sizeof(code));
        assert(emu.x[2] == 89); // 10th Fibonacci number
    }

        // TEST 14: F Extension - Basic Floating Point Arithmetic (FADD.S, FSUB.S, FMUL.S, FDIV.S)
    {
        unsigned char code[80] =
{
	0x93, 0x00, 0x00, 0x50, 0x37, 0x01, 0x40, 0x40, 
	0x23, 0xa0, 0x20, 0x00, 0x87, 0xa0, 0x00, 0x00, 
	0xb7, 0x01, 0x00, 0x40, 0x23, 0xa0, 0x30, 0x00, 
	0x07, 0xa1, 0x00, 0x00, 0xd3, 0xf1, 0x20, 0x00, 
	0x53, 0xf2, 0x20, 0x08, 0xd3, 0xf2, 0x20, 0x10, 
	0x53, 0xf3, 0x20, 0x18, 0x27, 0xa0, 0x30, 0x00, 
	0x03, 0xa5, 0x00, 0x00, 0x27, 0xa0, 0x40, 0x00, 
	0x83, 0xa5, 0x00, 0x00, 0x27, 0xa0, 0x50, 0x00, 
	0x03, 0xa6, 0x00, 0x00, 0x27, 0xa0, 0x60, 0x00, 
	0x83, 0xa6, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 
};

        /*
        # Setup: Load floating point values via memory
        addi x1, x0, 0x500   # x1 = data address
        lui x2, 0x40400      # x2 = 0x40400000 (3.0f)
        sw x2, 0(x1)
        flw f1, 0(x1)        # f1 = 3.0
        lui x3, 0x40000      # x3 = 0x40000000 (2.0f)
        sw x3, 0(x1)
        flw f2, 0(x1)        # f2 = 2.0
        
        fadd.s f3, f1, f2    # f3 = 3.0 + 2.0 = 5.0
        fsub.s f4, f1, f2    # f4 = 3.0 - 2.0 = 1.0
        fmul.s f5, f1, f2    # f5 = 3.0 * 2.0 = 6.0
        fdiv.s f6, f1, f2    # f6 = 3.0 / 2.0 = 1.5
        
        # Store results back to memory for verification
        fsw f3, 0(x1)
        lw x10, 0(x1)        # x10 = bits of 5.0
        fsw f4, 0(x1)
        lw x11, 0(x1)        # x11 = bits of 1.0
        fsw f5, 0(x1)
        lw x12, 0(x1)        # x12 = bits of 6.0
        fsw f6, 0(x1)
        lw x13, 0(x1)        # x13 = bits of 1.5
        ecall
        */
        
        mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
        run_test("F Extension - Basic Arithmetic", code, sizeof(code));
        
        float result;
        memcpy(&result, &emu.x[10], 4);
        assert(fabs(result - 5.0f) < 0.0001f);
        memcpy(&result, &emu.x[11], 4);
        assert(fabs(result - 1.0f) < 0.0001f);
        memcpy(&result, &emu.x[12], 4);
        assert(fabs(result - 6.0f) < 0.0001f);
        memcpy(&result, &emu.x[13], 4);
        assert(fabs(result - 1.5f) < 0.0001f);
    }
    
    // TEST 15: F Extension - Square Root and Sign Injection (FSQRT.S, FSGNJ.S, FSGNJN.S, FSGNJX.S)
    {
        unsigned char code[72] =
{
	0x93, 0x00, 0x00, 0x50, 0x37, 0x01, 0x80, 0x40, 
	0x23, 0xa0, 0x20, 0x00, 0x87, 0xa0, 0x00, 0x00, 
	0x53, 0xf1, 0x00, 0x58, 0xb7, 0x01, 0x00, 0xc0, 
	0x23, 0xa0, 0x30, 0x00, 0x87, 0xa1, 0x00, 0x00, 
	0x53, 0x82, 0x30, 0x20, 0xd3, 0x92, 0x30, 0x20, 
	0x53, 0xa3, 0x30, 0x20, 0x27, 0xa0, 0x20, 0x00, 
	0x03, 0xa5, 0x00, 0x00, 0x27, 0xa0, 0x40, 0x00, 
	0x83, 0xa5, 0x00, 0x00, 0x27, 0xa0, 0x50, 0x00, 
	0x03, 0xa6, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 
};

        /*
        addi x1, x0, 0x500   # x1 = data address
        lui x2, 0x40800      # x2 = 0x40800000 (4.0f)
        sw x2, 0(x1)
        flw f1, 0(x1)        # f1 = 4.0
        
        fsqrt.s f2, f1       # f2 = sqrt(4.0) = 2.0
        
        lui x3, 0xC0000      # x3 = 0xC0000000 (-2.0f)
        sw x3, 0(x1)
        flw f3, 0(x1)        # f3 = -2.0
        
        fsgnj.s f4, f1, f3   # f4 = abs(f1) * sign(f3) = -4.0
        fsgnjn.s f5, f1, f3  # f5 = abs(f1) * -sign(f3) = 4.0
        fsgnjx.s f6, f1, f3  # f6 = abs(f1) * (sign(f1) XOR sign(f3)) = -4.0
        
        # Store results
        fsw f2, 0(x1)
        lw x10, 0(x1)        # x10 = bits of 2.0
        fsw f4, 0(x1)
        lw x11, 0(x1)        # x11 = bits of -4.0
        fsw f5, 0(x1)
        lw x12, 0(x1)        # x12 = bits of 4.0
        ecall
        */
        
        mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
        run_test("F Extension - SQRT and Sign Injection", code, sizeof(code));
        
        float result;
        memcpy(&result, &emu.x[10], 4);
        assert(fabs(result - 2.0f) < 0.0001f);
        memcpy(&result, &emu.x[11], 4);
        assert(fabs(result - (-4.0f)) < 0.0001f);
        memcpy(&result, &emu.x[12], 4);
        assert(fabs(result - 4.0f) < 0.0001f);
    }
    
    // // TEST 16: F Extension - Min/Max and Comparisons (FMIN.S, FMAX.S, FEQ.S, FLT.S, FLE.S)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     lui x2, 0x40400      # x2 = 0x40400000 (3.0f)
    //     sw x2, 0(x1)
    //     flw f1, 0(x1)        # f1 = 3.0
    //     lui x3, 0x40A00      # x3 = 0x40A00000 (5.0f)
    //     sw x3, 0(x1)
    //     flw f2, 0(x1)        # f2 = 5.0
        
    //     fmin.s f3, f1, f2    # f3 = min(3.0, 5.0) = 3.0
    //     fmax.s f4, f1, f2    # f4 = max(3.0, 5.0) = 5.0
        
    //     feq.s x10, f1, f2    # x10 = (3.0 == 5.0) = 0
    //     feq.s x11, f1, f1    # x11 = (3.0 == 3.0) = 1
    //     flt.s x12, f1, f2    # x12 = (3.0 < 5.0) = 1
    //     flt.s x13, f2, f1    # x13 = (5.0 < 3.0) = 0
    //     fle.s x14, f1, f2    # x14 = (3.0 <= 5.0) = 1
    //     fle.s x15, f1, f1    # x15 = (3.0 <= 3.0) = 1
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("F Extension - Min/Max and Comparisons", code, sizeof(code));
        
    //     assert(emu.x[10] == 0);
    //     assert(emu.x[11] == 1);
    //     assert(emu.x[12] == 1);
    //     assert(emu.x[13] == 0);
    //     assert(emu.x[14] == 1);
    //     assert(emu.x[15] == 1);
    // }
    
    // // TEST 17: F Extension - Conversions (FCVT.W.S, FCVT.WU.S, FCVT.S.W, FCVT.S.WU)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     lui x2, 0x40400      # x2 = 0x40400000 (3.0f)
    //     sw x2, 0(x1)
    //     flw f1, 0(x1)        # f1 = 3.0
        
    //     fcvt.w.s x10, f1     # x10 = (int)3.0 = 3
    //     fcvt.wu.s x11, f1    # x11 = (unsigned)3.0 = 3
        
    //     addi x3, x0, 42      # x3 = 42
    //     fcvt.s.w f2, x3      # f2 = (float)42 = 42.0
    //     addi x4, x0, -5      # x4 = -5
    //     fcvt.s.w f3, x4      # f3 = (float)-5 = -5.0
        
    //     # Store float results
    //     fsw f2, 0(x1)
    //     lw x12, 0(x1)        # x12 = bits of 42.0
    //     fsw f3, 0(x1)
    //     lw x13, 0(x1)        # x13 = bits of -5.0
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("F Extension - Conversions", code, sizeof(code));
        
    //     assert(emu.x[10] == 3);
    //     assert(emu.x[11] == 3);
        
    //     float result;
    //     memcpy(&result, &emu.x[12], 4);
    //     assert(fabs(result - 42.0f) < 0.0001f);
    //     memcpy(&result, &emu.x[13], 4);
    //     assert(fabs(result - (-5.0f)) < 0.0001f);
    // }
    
    // // TEST 18: F Extension - Move and Classify (FMV.X.W, FMV.W.X, FCLASS.S)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     lui x2, 0x40000      # x2 = 0x40000000 (2.0f)
    //     sw x2, 0(x1)
    //     flw f1, 0(x1)        # f1 = 2.0
        
    //     fmv.x.w x10, f1      # x10 = bit pattern of f1
    //     fmv.w.x f2, x2       # f2 = interpret x2 as float
        
    //     fclass.s x11, f1     # x11 = class of 2.0 (positive normal)
        
    //     # Test with zero
    //     lui x3, 0            # x3 = 0 (0.0f)
    //     fmv.w.x f3, x3       # f3 = 0.0
    //     fclass.s x12, f3     # x12 = class of 0.0 (positive zero)
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("F Extension - Move and Classify", code, sizeof(code));
        
    //     assert(emu.x[10] == 0x40000000);
    //     assert((emu.x[11] & 0x080) != 0); // Positive normal number
    //     assert((emu.x[12] & 0x010) != 0); // Positive zero
    // }
    
    // // TEST 19: F Extension - Fused Multiply-Add (FMADD.S, FMSUB.S, FNMADD.S, FNMSUB.S)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     lui x2, 0x40000      # x2 = 0x40000000 (2.0f)
    //     sw x2, 0(x1)
    //     flw f1, 0(x1)        # f1 = 2.0
    //     lui x3, 0x40400      # x3 = 0x40400000 (3.0f)
    //     sw x3, 0(x1)
    //     flw f2, 0(x1)        # f2 = 3.0
    //     lui x4, 0x40800      # x4 = 0x40800000 (4.0f)
    //     sw x4, 0(x1)
    //     flw f3, 0(x1)        # f3 = 4.0
        
    //     fmadd.s f4, f1, f2, f3   # f4 = (2.0 * 3.0) + 4.0 = 10.0
    //     fmsub.s f5, f1, f2, f3   # f5 = (2.0 * 3.0) - 4.0 = 2.0
    //     fnmadd.s f6, f1, f2, f3  # f6 = -(2.0 * 3.0) + 4.0 = -2.0
    //     fnmsub.s f7, f1, f2, f3  # f7 = -(2.0 * 3.0) - 4.0 = -10.0
        
    //     # Store results
    //     fsw f4, 0(x1)
    //     lw x10, 0(x1)        # x10 = bits of 10.0
    //     fsw f5, 0(x1)
    //     lw x11, 0(x1)        # x11 = bits of 2.0
    //     fsw f6, 0(x1)
    //     lw x12, 0(x1)        # x12 = bits of -2.0
    //     fsw f7, 0(x1)
    //     lw x13, 0(x1)        # x13 = bits of -10.0
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("F Extension - Fused Multiply-Add", code, sizeof(code));
        
    //     float result;
    //     memcpy(&result, &emu.x[10], 4);
    //     assert(fabs(result - 10.0f) < 0.0001f);
    //     memcpy(&result, &emu.x[11], 4);
    //     assert(fabs(result - 2.0f) < 0.0001f);
    //     memcpy(&result, &emu.x[12], 4);
    //     assert(fabs(result - (-2.0f)) < 0.0001f);
    //     memcpy(&result, &emu.x[13], 4);
    //     assert(fabs(result - (-10.0f)) < 0.0001f);
    // }
    
    // // TEST 20: D Extension - Basic Double Precision Arithmetic (FADD.D, FSUB.D, FMUL.D, FDIV.D)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
        
    //     # Load 3.0 as double (0x4008000000000000)
    //     lui x2, 0x40080      
    //     sw x2, 4(x1)
    //     sw x0, 0(x1)
    //     fld f1, 0(x1)        # f1 = 3.0 (double)
        
    //     # Load 2.0 as double (0x4000000000000000)
    //     lui x3, 0x40000
    //     sw x3, 4(x1)
    //     sw x0, 0(x1)
    //     fld f2, 0(x1)        # f2 = 2.0 (double)
        
    //     fadd.d f3, f1, f2    # f3 = 3.0 + 2.0 = 5.0
    //     fsub.d f4, f1, f2    # f4 = 3.0 - 2.0 = 1.0
    //     fmul.d f5, f1, f2    # f5 = 3.0 * 2.0 = 6.0
    //     fdiv.d f6, f1, f2    # f6 = 3.0 / 2.0 = 1.5
        
    //     # Store results (just check they don't crash)
    //     fsd f3, 0(x1)
    //     fsd f4, 0(x1)
    //     fsd f5, 0(x1)
    //     fsd f6, 0(x1)
        
    //     addi x10, x0, 1      # Success indicator
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("D Extension - Basic Arithmetic", code, sizeof(code));
        
    //     // Verify doubles work (detailed verification would require reading from memory)
    //     assert(emu.x[10] == 1);
        
    //     // Read back and verify one result
    //     double result;
    //     mem.memcpy(&result, 0x500, 8);
    //     assert(fabs(result - 1.5) < 0.0001);
    // }
    
    // // TEST 21: D Extension - Conversions (FCVT.D.S, FCVT.S.D, FCVT.W.D, FCVT.D.W)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
        
    //     # Load single precision 3.0
    //     lui x2, 0x40400      # x2 = 0x40400000 (3.0f)
    //     sw x2, 0(x1)
    //     flw f1, 0(x1)        # f1 = 3.0 (single)
        
    //     fcvt.d.s f2, f1      # f2 = 3.0 (double)
    //     fcvt.s.d f3, f2      # f3 = 3.0 (single)
        
    //     fcvt.w.d x10, f2     # x10 = (int)3.0 = 3
        
    //     addi x3, x0, 42      # x3 = 42
    //     fcvt.d.w f4, x3      # f4 = (double)42 = 42.0
    //     fcvt.w.d x11, f4     # x11 = (int)42.0 = 42
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("D Extension - Conversions", code, sizeof(code));
        
    //     assert(emu.x[10] == 3);
    //     assert(emu.x[11] == 42);
    // }
    
    // // TEST 22: D Extension - Comparisons and Min/Max (FEQ.D, FLT.D, FLE.D, FMIN.D, FMAX.D)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
        
    //     # Load 3.0 as double
    //     lui x2, 0x40080      
    //     sw x2, 4(x1)
    //     sw x0, 0(x1)
    //     fld f1, 0(x1)        # f1 = 3.0
        
    //     # Load 5.0 as double
    //     lui x3, 0x40140
    //     sw x3, 4(x1)
    //     sw x0, 0(x1)
    //     fld f2, 0(x1)        # f2 = 5.0
        
    //     feq.d x10, f1, f2    # x10 = (3.0 == 5.0) = 0
    //     feq.d x11, f1, f1    # x11 = (3.0 == 3.0) = 1
    //     flt.d x12, f1, f2    # x12 = (3.0 < 5.0) = 1
    //     fle.d x13, f1, f2    # x13 = (3.0 <= 5.0) = 1
        
    //     fmin.d f3, f1, f2    # f3 = min(3.0, 5.0) = 3.0
    //     fmax.d f4, f1, f2    # f4 = max(3.0, 5.0) = 5.0
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("D Extension - Comparisons", code, sizeof(code));
        
    //     assert(emu.x[10] == 0);
    //     assert(emu.x[11] == 1);
    //     assert(emu.x[12] == 1);
    //     assert(emu.x[13] == 1);
    // }
    
    // // TEST 23: A Extension - Atomic Memory Operations (LR.W, SC.W)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     addi x2, x0, 42      # x2 = 42
    //     sw x2, 0(x1)         # Store 42 at address
        
    //     lr.w x10, (x1)       # x10 = 42, set reservation
    //     addi x3, x0, 100     # x3 = 100
    //     sc.w x11, x3, (x1)   # Store 100, x11 = 0 (success)
    //     lw x12, 0(x1)        # x12 = 100 (verify store)
        
    //     # Try SC without LR (should fail)
    //     addi x4, x0, 200     # x4 = 200
    //     sc.w x13, x4, (x1)   # x13 = 1 (failure - no reservation)
    //     lw x14, 0(x1)        # x14 = 100 (unchanged)
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("A Extension - LR/SC", code, sizeof(code));
        
    //     assert(emu.x[10] == 42);
    //     assert(emu.x[11] == 0);  // SC succeeded
    //     assert(emu.x[12] == 100);
    //     assert(emu.x[13] == 1);  // SC failed (no reservation)
    //     assert(emu.x[14] == 100);
    // }
    
    // // TEST 24: A Extension - Atomic Swap and Add (AMOSWAP.W, AMOADD.W)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     addi x2, x0, 50      # x2 = 50
    //     sw x2, 0(x1)         # Store 50 at address
        
    //     addi x3, x0, 25      # x3 = 25
    //     amoswap.w x10, x3, (x1)  # x10 = 50 (old value), mem = 25
    //     lw x11, 0(x1)        # x11 = 25 (verify)
        
    //     addi x4, x0, 15      # x4 = 15
    //     amoadd.w x12, x4, (x1)   # x12 = 25 (old value), mem = 25+15 = 40
    //     lw x13, 0(x1)        # x13 = 40 (verify)
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("A Extension - SWAP and ADD", code, sizeof(code));
        
    //     assert(emu.x[10] == 50);
    //     assert(emu.x[11] == 25);
    //     assert(emu.x[12] == 25);
    //     assert(emu.x[13] == 40);
    // }
    
    // // TEST 25: A Extension - Atomic Logical Operations (AMOXOR.W, AMOAND.W, AMOOR.W)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     addi x2, x0, 0xFF    # x2 = 0xFF
    //     sw x2, 0(x1)         # Store 0xFF at address
        
    //     addi x3, x0, 0xF0    # x3 = 0xF0
    //     amoxor.w x10, x3, (x1)   # x10 = 0xFF, mem = 0xFF XOR 0xF0 = 0x0F
    //     lw x11, 0(x1)        # x11 = 0x0F
        
    //     addi x4, x0, 0x0F    # x4 = 0x0F
    //     sw x2, 0(x1)         # Reset to 0xFF
    //     amoand.w x12, x4, (x1)   # x12 = 0xFF, mem = 0xFF AND 0x0F = 0x0F
    //     lw x13, 0(x1)        # x13 = 0x0F
        
    //     addi x5, x0, 0xF0    # x5 = 0xF0
    //     amoor.w x14, x5, (x1)    # x14 = 0x0F, mem = 0x0F OR 0xF0 = 0xFF
    //     lw x15, 0(x1)        # x15 = 0xFF
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("A Extension - Logical Atomics", code, sizeof(code));
        
    //     assert(emu.x[10] == 0xFF);
    //     assert(emu.x[11] == 0x0F);
    //     assert(emu.x[12] == 0xFF);
    //     assert(emu.x[13] == 0x0F);
    //     assert(emu.x[14] == 0x0F);
    //     assert(emu.x[15] == 0xFF);
    // }
    
    // // TEST 26: A Extension - Atomic Min/Max (AMOMIN.W, AMOMAX.W, AMOMINU.W, AMOMAXU.W)
    // {
    //     uint32_t code[128];
    //     /*
    //     addi x1, x0, 0x500   # x1 = data address
    //     addi x2, x0, 10      # x2 = 10
    //     sw x2, 0(x1)         # Store 10 at address
        
    //     addi x3, x0, 5       # x3 = 5
    //     amomin.w x10, x3, (x1)   # x10 = 10, mem = min(10, 5) = 5
    //     lw x11, 0(x1)        # x11 = 5
        
    //     addi x4, x0, 20      # x4 = 20
    //     amomax.w x12, x4, (x1)   # x12 = 5, mem = max(5, 20) = 20
    //     lw x13, 0(x1)        # x13 = 20
        
    //     addi x5, x0, -1      # x5 = -1 (0xFFFFFFFF)
    //     amomin.w x14, x5, (x1)   # x14 = 20, mem = min(20, -1) = -1 (signed)
    //     lw x15, 0(x1)        # x15 = 0xFFFFFFFF
        
    //     addi x6, x0, 10      # x6 = 10
    //     amominu.w x16, x6, (x1)  # x16 = 0xFFFFFFFF, mem = min(0xFFFFFFFF, 10) = 10 (unsigned)
    //     lw x17, 0(x1)        # x17 = 10
    //     ecall
    //     */
        
    //     mem.map_anonymous(0x500, 0x1000, PERM_READ | PERM_WRITE);
        
    //     run_test("A Extension - Min/Max Atomics", code, sizeof(code));
        
    //     assert(emu.x[10] == 10);
    //     assert(emu.x[11] == 5);
    //     assert(emu.x[12] == 5);
    //     assert(emu.x[13] == 20);
    //     assert(emu.x[14] == 20);
    //     assert(emu.x[15] == (uint32_t)-1);
    //     assert(emu.x[16] == (uint32_t)-1);
    //     assert(emu.x[17] == 10);
    // }
}
