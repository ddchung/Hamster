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
        int load_elf32(File file, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk, uint64_t &phdr_loc)
        {
            mem_space.unmap_all();

            if (file.seek(0, H_SEEK_SET) < 0)
            {
                error = H_EIO;
                return -1;
            }

            // Read ELF header
            Elf32_Ehdr ehdr;

            if (file.read(&ehdr, sizeof(ehdr)) != sizeof(ehdr))
            {
                error = H_EIO;
                return -1;
            }

            // Check ELF header
            if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0)
            {
                error = H_ENOEXEC;
                return -1;
            }

            if (ehdr.e_ident[EI_CLASS] != ELFCLASS32)
            {
                error = H_ENOEXEC;
                return -1;
            }

            if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB)
            {
                error = H_ENOEXEC;
                return -1;
            }

            if (ehdr.e_type != ET_EXEC)
            {
                // TODO: Dynamic linking is not supported yet
                error = H_ENOEXEC;
                return -1;
            }

            if (ehdr.e_machine != EM_RISCV)
            {
                error = H_ENOEXEC;
                return -1;
            }

            entry_point = ehdr.e_entry;
            ph_num = ehdr.e_phnum;
            brk = 0;

            // Load program headers

            for (int i = 0; i < ehdr.e_phnum; ++i)
            {
                Elf32_Phdr phdr;

                if (file.seek(ehdr.e_phoff + i * ehdr.e_phentsize, H_SEEK_SET) < 0)
                {
                    error = H_EIO;
                    return -1;
                }

                if (file.read(&phdr, sizeof(phdr)) != sizeof(phdr))
                {
                    error = H_EIO;
                    return -1;
                }

                if (phdr.p_type == PT_LOAD)
                {
                    if (phdr.p_offset == 0)
                        phdr_loc = phdr.p_vaddr + ehdr.e_phoff;

                    // Load segment
                    if (file.seek(phdr.p_offset, H_SEEK_SET) < 0)
                    {
                        error = H_EIO;
                        return -1;
                    }

                    uint64_t top = phdr.p_vaddr + phdr.p_memsz;
                    if (top > brk)
                        brk = top;
                    
                    // Map segment

                    // TODO: handle error
                    if (mem_space.map_private_file(phdr.p_vaddr, file.get_fd(), phdr.p_offset, phdr.p_filesz, phdr.p_flags & 07) < 0)
                        return -1;
                    
                    if (phdr.p_memsz > phdr.p_filesz)
                    {
                        // Zero out rest
                        if (mem_space.memset_alloc(phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz) < 0)
                        {
                            error = H_EIO;
                            return -1;
                        }
                    }
                }
            }

            brk = (brk + (HAMSTER_PAGE_SIZE - 1)) & ~((uint64_t)HAMSTER_PAGE_SIZE - 1);

            // done loading
            return 0;
        }
    } // namespace
    

    int load_elf(File file, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk, uint64_t &phdr_loc)
    {
        // Prepare file
        if (file.seek(0, H_SEEK_SET) < 0)
        {
            error = H_EIO;
            return -1;
        }

        // Read ELF e_ident
        uint8_t e_ident[EI_NIDENT];

        if (file.read(e_ident, EI_NIDENT) != EI_NIDENT)
        {
            error = H_EIO;
            return -1;
        }

        // Check ELF magic number
        if (memcmp(e_ident, ELFMAG, SELFMAG) != 0)
        {
            error = H_ENOEXEC;
            return -1;
        }

        if (e_ident[EI_CLASS] == ELFCLASS32)
        {
            return load_elf32(file, mem_space, entry_point, ph_num, brk, phdr_loc);
        }
        else
        {
            error = H_ENOEXEC;
            return -1;
        }
    }
} // namespace Hamster


