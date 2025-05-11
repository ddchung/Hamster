#include <platform/platform.hpp>
#include <memory/allocator.hpp>

#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#endif

void test_platform();
void test_memory();
void test_filesystem();

int main()
{
  Hamster::_init_platform();

  Hamster::_log("Testing Platform...\n");
  test_platform();
  Hamster::_log("Done\n");

  Hamster::_log("Testing Memory...\n");
  test_memory();
  Hamster::_log("Done\n");

  Hamster::_log("Testing Filesystem...\n");
  test_filesystem();
  Hamster::_log("Done\n");
}
