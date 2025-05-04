#include <platform/platform.hpp>
#include <filesystem/mounts.hpp>
#include <filesystem/ramfs.hpp>
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
  Hamster::_log("Mounting filesystems...\n");
  if (Hamster::_mount_rootfs() < 0)
  {
    Hamster::_log("Failed to mount root filesystem\n");
    return -1;
  }

  Hamster::dealloc(Hamster::mounts.mkdir("/tmp", O_RDWR, 0777));
  if (Hamster::mounts.mount(Hamster::alloc<Hamster::RamFs>(), "/tmp") < 0)
  {
    Hamster::_log("Failed to mount ramfs on /tmp\n");
    return -1;
  }
  Hamster::_log("Done\n");

  // make test files
  Hamster::_log("Creating test files...\n");
  auto file = Hamster::mounts.mkfile("/test.txt", O_RDWR, 0777);
  if (!file)
  {
    Hamster::_log("Failed to create test file\n");
    return -1;
  }

  file->write((const uint8_t *)"Hello, World!", 13);
  Hamster::_log("Test file created\n");

  Hamster::dealloc(file);

  // List files
  Hamster::_log("Listing files...\n");
  Hamster::BaseFile *file2 = Hamster::mounts.open("/", O_RDONLY);
  if (!file2 || file2->type() != Hamster::FileType::Directory)
  {
    Hamster::_log("Failed to open root directory\n");
    return -1;
  }
  Hamster::BaseDirectory *dir = static_cast<Hamster::BaseDirectory *>(file2);
  
  char * const *files = dir->list();
  if (!files)
  {
    Hamster::_log("Failed to list files\n");
    return -1;
  }

  for (int i = 0; files[i]; ++i)
  {
    Hamster::BaseFile *file = dir->open(files[i], O_RDONLY);
    if (!file)
    {
      Hamster::_log("Failed to open file '");
      Hamster::_log(files[i]);
      Hamster::_log("'\n");
      continue;
    }

    Hamster::FileType type = file->type();
    int mode = file->get_mode();

    if (type == Hamster::FileType::Directory) Hamster::_log("d"); else Hamster::_log("-");
    if (mode & 0b100000000) Hamster::_log("r"); else Hamster::_log("-");
    if (mode & 0b010000000) Hamster::_log("w"); else Hamster::_log("-");
    if (mode & 0b001000000) Hamster::_log("x"); else Hamster::_log("-");
    if (mode & 0b000100000) Hamster::_log("r"); else Hamster::_log("-");
    if (mode & 0b000010000) Hamster::_log("w"); else Hamster::_log("-");
    if (mode & 0b000001000) Hamster::_log("x"); else Hamster::_log("-");
    if (mode & 0b000000100) Hamster::_log("r"); else Hamster::_log("-");
    if (mode & 0b000000010) Hamster::_log("w"); else Hamster::_log("-");
    if (mode & 0b000000001) Hamster::_log("x"); else Hamster::_log("-");

    Hamster::_log("\t");

    Hamster::_log(files[i]);

    Hamster::_log("\n");
    
    Hamster::dealloc(file);
    Hamster::dealloc(files[i]);
  }
  Hamster::dealloc(files);
  Hamster::dealloc(dir);
  Hamster::_log("Done\n");
}
