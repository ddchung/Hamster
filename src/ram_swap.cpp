// Ram-resident swap

// shouldn't be used on the limited memory of Arduino's and compatibles
// override here  ------------v
#if !defined(ARDUINO) && 1 || 0

#include <platform/platform.hpp>
#include <platform/config.hpp>
#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace
{
    struct SwapPage
    {
        uint8_t data[HAMSTER_PAGE_SIZE];
    };

    std::unordered_map<int, SwapPage> swapped_pages;
} // namespace

int Hamster::_swap_out(int index, const uint8_t *data)
{
    if (index < 0)
        return -1;
    uint8_t *dest = swapped_pages[index].data;
    memcpy(dest, data, HAMSTER_PAGE_SIZE);
    return 0;
}

int Hamster::_swap_in(int index, uint8_t *dest)
{
    if (index < 0)
        return -1;
    
    auto end = swapped_pages.end();
    auto item = swapped_pages.find(index);
    if (item == end) // not found
        return -2;
    
    const uint8_t *src = (*item).second.data;
    memcpy(dest, src, HAMSTER_PAGE_SIZE);

    return 0;
}

int Hamster::_swap_rm(int index)
{
    return swapped_pages.erase(index) == 1 ? 0 : -1;
}

int Hamster::_swap_rm_all()
{
    swapped_pages.clear();
    return 0;
}

#endif

