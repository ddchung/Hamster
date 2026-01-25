
#include <process/scatter_io.hpp>
#include <errno/errno.h>
#include <cassert>

namespace Hamster
{
    namespace
    {
        const std::pair<IOVec *, size_t> ERR_PAIR = std::make_pair(nullptr, 0);
        
        uint32_t iovec_count(uint32_t buf_loc, uint32_t buf_size)
        {
            if (buf_size == 0)
                return 0;
            uint32_t end_page = (buf_loc + buf_size - 1) >> HAMSTER_PAGE_SIZE_BITS;
            uint32_t start_page = (buf_loc) >> HAMSTER_PAGE_SIZE_BITS;
            return end_page - start_page + 1;
        }

        int check_perms(Task &task, uint32_t buf_loc, uint32_t buf_size, uint8_t perms)
        {
            int8_t mem_perms = task.mem_get_permissions(buf_loc, buf_size);
            if (mem_perms < 0 || task.mem_is_mapped(buf_loc, buf_size) != 1 || ((perms | PERM_READ) & ~mem_perms) != 0)
            {
                error = H_EFAULT;
                return -1;
            }
            return 0;
        }
    } // namespace
    

    std::pair<IOVec *, size_t> make_iovec_buf(Task &task, uint32_t buf_loc, uint32_t buf_size, uint8_t perms)
    {
        if (check_perms(task, buf_loc, buf_size, perms) < 0)
        {
            error = H_EFAULT;
            return ERR_PAIR;
        }

        // Count the number of pages the buffer occupies
        uint32_t iov_cnt = iovec_count(buf_loc, buf_size);
        
        IOVec *iovec = alloc<IOVec>(iov_cnt);

        for (uint32_t it = 0, addr = buf_loc; addr < buf_loc + buf_size; (++it, addr = (addr + HAMSTER_PAGE_SIZE) & ~(HAMSTER_PAGE_SIZE-1)))
        {
            void *p = (void *)task.mem_make_iterator_read(addr);
            if (!p)
            {
                dealloc(iovec);
                error = H_EFAULT;
                return ERR_PAIR;
            }
            iovec[it].data = p;
            iovec[it].size = std::min<uint32_t>(buf_loc + buf_size - addr, HAMSTER_PAGE_SIZE);
        }
        return std::make_pair(iovec, iov_cnt);
    }

    std::pair<IOVec *, size_t> make_iovec(Task &task, const sys_iovec *iovec, size_t iovec_len, uint8_t perms)
    {
        uint32_t iov_cnt = 0;
        for (const sys_iovec *it = iovec; it - iovec < (ptrdiff_t)iovec_len; ++it)
        {
            if (check_perms(task, it->data, it->size, perms) < 0)
                return ERR_PAIR;
            iov_cnt += iovec_count(it->data, it->size);
        }

        IOVec *iov_buf = alloc<IOVec>(iov_cnt);

        IOVec *iov_it = iov_buf;
        for (const sys_iovec *it = iovec; it - iovec < (ptrdiff_t)iovec_len; ++it)
        {
            if (it->size == 0)
                continue;
            auto pair = make_iovec_buf(task, it->data, it->size, perms);
            if (pair.first == nullptr)
            {
                // TODO: Will this happen?
                dealloc(iov_buf);
                return ERR_PAIR;
            }
            assert(pair.second == iovec_count(it->data, it->size));
            memcpy(iov_it, pair.first, pair.second * sizeof(IOVec));
            dealloc(pair.first);
            iov_it += pair.second;
        }

        assert(iov_it - iov_buf == iov_cnt);

        return std::make_pair(iov_buf, iov_cnt);
    }

    std::pair<IOVec *, size_t> make_iovec(Task &task, uint32_t iovec_loc, uint32_t iovec_count, uint8_t perms)
    {
        if ((iovec_loc >> HAMSTER_PAGE_SIZE_BITS) ==
            ((iovec_loc + iovec_count * sizeof(sys_iovec) - 1) >> HAMSTER_PAGE_SIZE_BITS))
        {
            // Entire iovector is in one page
            const sys_iovec *vecs = (const sys_iovec *)task.mem_make_iterator_read(iovec_loc);
            return make_iovec(task, vecs, iovec_count, perms);
        }
        else
        {
            // Split across pages, allocate buffer and copy
            sys_iovec *buf = alloc<sys_iovec>(iovec_count);
            if (task.memcpy(buf, iovec_loc, iovec_count * sizeof(sys_iovec)) < 0)
            {
                dealloc(buf);
                return ERR_PAIR;
            }
            
            auto res = make_iovec(task, buf, iovec_count, perms);
            dealloc(buf);
            return res;
        }
    }
} // namespace Hamster
