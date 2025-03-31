// Test platform-specific code

#include <platform/platform.hpp>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <iostream>

uint8_t buffer1[HAMSTER_PAGE_SIZE];
uint8_t buffer2[HAMSTER_PAGE_SIZE];

template <typename T>
static void assert_equal(const T &a, const T &b, const char *file, int line)
{
    if (a == b)
        return;
    std::cerr << "Assertion failed: " << file << ":" << line << ": "
              << "Expected: " << a << ", but got: " << b << std::endl;
    while (1)
        ;
}

#define assert_eq(a, b) assert_equal(a, b, __FILE__, __LINE__)

void test_platform()
{
    int i;
    // Test malloc and free
    void *ptr = Hamster::_malloc(100);
    assert(ptr != nullptr);
    i = Hamster::_free(ptr);
    assert_eq(i, 0);

    // Test swap out and in

    // sequential
    for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
    {
        buffer1[i] = i;
    }
    i = Hamster::_swap_out(0, buffer1);
    assert_eq(i, 0);

    i = Hamster::_swap_in(0, buffer2);
    assert_eq(i, 0);

    // Check that buffer2 contains the same data as buffer1
    for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
    {
        assert_eq(buffer1[i], buffer2[i]);
    }

    // random
    for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
    {
        buffer1[i] = rand() % 256;
    }

    i = Hamster::_swap_out(1, buffer1);
    assert_eq(i, 0);
    i = Hamster::_swap_in(1, buffer2);
    assert_eq(i, 0);

    // Check that buffer2 contains the same data as buffer1
    for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
    {
        assert_eq(buffer1[i], buffer2[i]);
    }

    // Test swap out and in with invalid index
    i = Hamster::_swap_out(-1, buffer1);
    assert(i < 0);
    i = Hamster::_swap_in(-1, buffer2);
    assert(i < 0);
}
