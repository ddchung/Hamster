#include <platform/platform.hpp>
#include <filesystem/mounts.hpp>
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

  // mount rootfs
  Hamster::_log("Mounting root filesystem...\n");
  if (Hamster::_mount_rootfs() < 0)
  {
    Hamster::_log("Failed to mount root filesystem\n");
    return -1;
  }
  Hamster::_log("Root filesystem mounted\n");

  // make test files
  Hamster::_log("Creating test files...\n");
  auto file = Hamster::Mounts::instance().mkfile("/test.txt", O_RDWR, 0);
  if (!file)
  {
    Hamster::_log("Failed to create test file\n");
    return -1;
  }

  file->write((const uint8_t *)"Hello, World!", 13);
  Hamster::_log("Test file created\n");

  Hamster::dealloc(file);
}
