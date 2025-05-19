#include <platform/platform.hpp>
#include <memory/allocator.hpp>
#include <memory/memory_space.hpp>
#include <process/riscv_rv32i_thread.hpp>
#include <process/thread_type_manager.hpp>
#include <elf/elf_loader.hpp>
#include <filesystem/vfs.hpp>
#include <filesystem/ramfs.hpp>

void test_platform();
void test_memory();
void test_filesystem();
void test_process();

extern "C" const unsigned char *riscv_prog_elf_ptr;
extern "C" const unsigned int riscv_prog_elf_size;

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

  Hamster::_log("Testing Process...\n");
  test_process();
  Hamster::_log("Done\n");

  // Test RISC-V RV32I Thread
  Hamster::MemorySpace mem_space;

  Hamster::vfs.mount("/", Hamster::alloc<Hamster::RamFs>());

  // Write ELF file to file
  int f = Hamster::vfs.mkfile("/test.elf", O_RDWR, 0777);
  if (f < 0)
  {
    printf("Failed to create file\n");
    return -1;
  }

  Hamster::vfs.write(f, riscv_prog_elf_ptr, riscv_prog_elf_size);
  Hamster::vfs.close(f);

  uint64_t entry_point = 0;
  uint64_t iterations = 0;
  f = Hamster::vfs.open("/test.elf", O_RDONLY);
  if (f < 0)
  {
    printf("Failed to open file\n");
    return -1;
  }
  auto thread = Hamster::thread_type_manager.load_elf(f, mem_space);
  Hamster::vfs.close(f);

  while (true)
  {
    int ret = thread->tick();
	++iterations;
    if (ret == 0)
      continue;
    if (ret < 0)
    {
      printf("Thread exited with error code %d\n", ret);
      break;
    }
    if (ret == 2)
    {
      // System call
      auto syscall = thread->get_syscall();
      if (syscall.syscall_num == 0)
      {
        printf("Thread exited with code %lu\n", syscall.arg1);
        break;
      }
      if (syscall.syscall_num == 1)
      {
        printf("getting 12345\n");
        // return 12345
        thread->set_syscall_ret(12345);
        continue;
      }
      else
      {
        printf("Unknown syscall %lu\n", syscall.syscall_num);
        continue;
      }
    }
  }

  printf("Iterations: %lu\n", iterations);

  Hamster::dealloc(thread);
}
