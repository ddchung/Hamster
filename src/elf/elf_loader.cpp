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
        int load_elf32(File file, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk)
        {
            mem_space.unmap_all();

            if (file.seek(0, H_SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }

            // Read ELF header
            Elf32_Ehdr ehdr;

            if (file.read(&ehdr, sizeof(ehdr)) != sizeof(ehdr))
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
            brk = 0;

            // Load program headers
            if (file.seek(ehdr.e_phoff, H_SEEK_SET) < 0)
            {
                error = EIO;
                return -1;
            }

            for (int i = 0; i < ehdr.e_phnum; ++i)
            {
                Elf32_Phdr phdr;

                if (file.seek(ehdr.e_phoff + i * ehdr.e_phentsize, H_SEEK_SET) < 0)
                {
                    error = EIO;
                    return -1;
                }

                if (file.read(&phdr, sizeof(phdr)) != sizeof(phdr))
                {
                    error = EIO;
                    return -1;
                }

                // Copy the program header to the memory space
                if (mem_space.memcpy_alloc(HAMSTER_STACK_TOP + 1 + i * sizeof(phdr), &phdr, sizeof(phdr)) != 0)
                {
                    error = EIO;
                    return -1;
                }

                if (phdr.p_type == PT_LOAD)
                {
                    // Load segment
                    if (file.seek(phdr.p_offset, H_SEEK_SET) < 0)
                    {
                        error = EIO;
                        return -1;
                    }

                    uint64_t top = phdr.p_vaddr + phdr.p_memsz;
                    if (top > brk)
                        brk = top;
                    
                    // Map segment

                    _trace("%s:%d load_elf32: mapping private region, vaddr=0x%08x, fd=%d, offset=%d, filesz=0x%08x\n", __FILE__, __LINE__, phdr.p_vaddr, file.get_fd(), phdr.p_offset, phdr.p_filesz);
                    int res = mem_space.map_private_file(phdr.p_vaddr, file.get_fd(), phdr.p_offset, phdr.p_filesz, phdr.p_flags & 07);
                    if (res < 0)
                        _trace("%s:%d load_elf32: map_private_file failed: error %d\n", __FILE__, __LINE__, error);
                    
                    if (phdr.p_memsz > phdr.p_filesz)
                    {
                        // Zero out rest
                        if (mem_space.memset_alloc(phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz) < 0)
                        {
                            error = EIO;
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
    

    int load_elf(File file, MemorySpace& mem_space, uint64_t& entry_point, uint64_t &ph_num, uint64_t &brk)
    {
        // Prepare file
        if (file.seek(0, H_SEEK_SET) < 0)
        {
            error = EIO;
            return -1;
        }

        // Read ELF e_ident
        uint8_t e_ident[EI_NIDENT];

        if (file.read(e_ident, EI_NIDENT) != EI_NIDENT)
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
            return load_elf32(file, mem_space, entry_point, ph_num, brk);
        }
        else
        {
            error = ENOEXEC;
            return -1;
        }
    }
} // namespace Hamster


