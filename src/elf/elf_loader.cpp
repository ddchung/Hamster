// ELF loader

#include <elf/elf_loader.hpp>
#include <filesystem/vfs.hpp>
#include <errno/errno.h>
#include <cstring>
#include <elf.h>

namespace Hamster
{
    namespace
    {
        int load_elf32(int fd, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk)
        {
            if (vfs.seek(fd, 0, H_SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }

            // Read ELF header
            Elf32_Ehdr ehdr;

            if (vfs.read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr))
            {
                error = EIO;
                return -1;
            }

            // Check ELF header
            if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0)
            {
                error = ENOEXEC;
                return -1;
            }

            if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
            {
                error = ENOEXEC;
                return -1;
            }

            if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
            {
                error = ENOEXEC;
                return -1;
            }

            if (ehdr.e_type != ET_EXEC)
            {
                // TODO: Dynamic linking is not supported yet
                error = ENOEXEC;
                return -1;
            }

            if (ehdr.e_machine != EM_RISCV)
            {
                error = ENOEXEC;
                return -1;
            }

            entry_point = ehdr.e_entry;
            ph_num = ehdr.e_phnum;

            // Load program headers
            if (vfs.seek(fd, ehdr.e_phoff, H_SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }

            for (int i = 0; i < ehdr.e_phnum; ++i)
            {
                Elf32_Phdr phdr;
                
                if (vfs.seek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, H_SEEK_SET) < 0)
                {
                    error = EIO;
                    return -1;
                }

                if (vfs.read(fd, &phdr, sizeof(phdr)) != sizeof(phdr))
                {
                    error = EIO;
                    return -1;
                }

                // Copy the program header to the memory space
                if (mem_space.memcpy(HAMSTER_STACK_TOP + 1 + i * sizeof(phdr), &phdr, sizeof(phdr)) != 0)
                {
                    error = EIO;
                    return -1;
                }

                brk = 0;

                if (phdr.p_type == PT_LOAD)
                {
                    // Load segment
                    if (vfs.seek(fd, phdr.p_offset, H_SEEK_SET) < 0)
                    {
                        error = EIO;
                        return -1;
                    }

                    uint64_t top = phdr.p_vaddr + phdr.p_memsz;
                    if (top > brk)
                        brk = top;

                    static uint8_t buf[64];
                    size_t bytes_to_read = phdr.p_filesz;
                    size_t bytes_read = 0;
                    while (bytes_to_read > 0)
                    {
                        size_t chunk_size = bytes_to_read < sizeof(buf) ? bytes_to_read : sizeof(buf);
                        ssize_t ret = vfs.read(fd, buf, chunk_size);
                        if (ret < 0)
                        {
                            error = EIO;
                            return -1;
                        }

                        mem_space.memcpy(phdr.p_vaddr + bytes_read, buf, ret);
                        bytes_read += ret;
                        bytes_to_read -= ret;
                    }

                    // Zero out the rest of the segment
                    if (phdr.p_memsz > phdr.p_filesz)
                    {
                        size_t zero_size = phdr.p_memsz - phdr.p_filesz;
                        mem_space.memset(phdr.p_vaddr + bytes_read, 0, zero_size);
                    }
                }
            }

            brk = (brk + (HAMSTER_PAGE_SIZE - 1)) & ~((uint64_t)HAMSTER_PAGE_SIZE - 1);

            // done loading
            return 0;
        }
    } // namespace
    

    int load_elf(int fd, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk)
    {
        // Prepare file
        if (vfs.seek(fd, 0, H_SEEK_SET) < 0)
        {
            error = EIO;
            return -1;
        }

        // Read ELF e_ident
        uint8_t e_ident[EI_NIDENT];

        if (vfs.read(fd, e_ident, EI_NIDENT) != EI_NIDENT)
        {
            error = EIO;
            return -1;
        }

        // Check ELF magic number
        if (memcmp(e_ident, ELFMAG, SELFMAG) != 0)
        {
            error = ENOEXEC;
            return -1;
        }

        if (e_ident[EI_CLASS] == ELFCLASS32)
        {
            return load_elf32(fd, mem_space, entry_point, ph_num, brk);
        }
        else
        {
            error = ENOEXEC;
            return -1;
        }
    }
} // namespace Hamster


