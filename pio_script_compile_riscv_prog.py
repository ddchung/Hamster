# PlatformIO build hook that compiles a RISC-V program, and loads it into a C array for building

import os

os.remove("src/riscv_prog_elf.c") if os.path.exists("src/riscv_prog_elf.c") else None

# Compile `riscv_prog.c` to an ELF file
os.system("bash -ic \"riscv32-unknown-elf-gcc -ffreestanding -O3 -c riscv_prog.c -o riscv_prog.o\"")
os.system("bash -ic \"riscv32-unknown-elf-gcc -ffreestanding -O3 -c riscv_start.c -o riscv_start.o\"")

# Link
os.system("bash -ic \"riscv32-unknown-elf-gcc -ffreestanding -nostdlib riscv_prog.o riscv_start.o -lc -o riscv_prog.elf\"")

# Read the ELF file and convert it to a C array

with open("riscv_prog.elf", "rb") as f:
    data = f.read()
    data = bytearray(data)
    data = [f"0x{byte:02x}" for byte in data]
    data = ", ".join(data)
    data = f"const unsigned char riscv_prog_elf[] = {{{data}}};\n"
    data += f"const unsigned int riscv_prog_elf_size = sizeof(riscv_prog_elf);\n"
    data += f"const unsigned char *riscv_prog_elf_ptr = riscv_prog_elf;\n"

with open("src/riscv_prog_elf.c", "w") as f:
    f.write(data)

