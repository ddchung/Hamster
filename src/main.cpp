#include <platform/platform.hpp>
#include <memory/allocator.hpp>
#include <memory/memory_space.hpp>
#include <process/riscv_rv32i_thread.hpp>

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

  // Test RISC-V RV32I Thread
  Hamster::MemorySpace mem_space;

  // Note: This program was compiled with RISC-V GCC, built from
  // crosstool-ng
  // Program is below.
  /*
  int syscall0(int syscall)
  {
      int ret;
      asm volatile(
          "mv a7, %1\n"
          "ecall\n"
          "mv %0, a0"
          : "=r"(ret)
          : "r"(syscall)
          : "a7", "a0"
      );
      return ret;
  }

  int syscall1(int syscall, int arg1)
  {
      int ret;
      asm volatile(
          "mv a7, %1\n"
          "mv a0, %2\n"
          "ecall\n"
          "mv %0, a0"
          : "=r"(ret)
          : "r"(syscall), "r"(arg1)
          : "a7", "a0"
      );
      return ret;
  }

  int main(void);

  void _start()
  {
      // Startup code
      
      int i = main();
      syscall1(0, i); // Exit syscall
  }

  int main(void)
  {
      int i = syscall0(1);
      return i;
  }
  */
  unsigned char prog[164] =
  {
    0x93, 0x07, 0x10, 0x00, 0x93, 0x88, 0x07, 0x00, 
    0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x05, 0x00, 
    0x13, 0x85, 0x07, 0x00, 0x67, 0x80, 0x00, 0x00, 
    0x93, 0x07, 0x05, 0x00, 0x93, 0x88, 0x07, 0x00, 
    0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x05, 0x00, 
    0x13, 0x85, 0x07, 0x00, 0x67, 0x80, 0x00, 0x00, 
    0x93, 0x07, 0x05, 0x00, 0x93, 0x88, 0x07, 0x00, 
    0x13, 0x85, 0x05, 0x00, 0x73, 0x00, 0x00, 0x00, 
    0x93, 0x07, 0x05, 0x00, 0x13, 0x85, 0x07, 0x00, 
    0x67, 0x80, 0x00, 0x00, 0x93, 0x07, 0x05, 0x00, 
    0x13, 0x87, 0x05, 0x00, 0x93, 0x88, 0x07, 0x00, 
    0x13, 0x05, 0x07, 0x00, 0x93, 0x05, 0x06, 0x00, 
    0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x05, 0x00, 
    0x13, 0x85, 0x07, 0x00, 0x67, 0x80, 0x00, 0x00, 
    0x67, 0x80, 0x00, 0x00, 0x13, 0x01, 0x01, 0xff, 
    0x23, 0x26, 0x11, 0x00, 0xef, 0xf0, 0x5f, 0xf8, 
    0x13, 0x07, 0x05, 0x00, 0x93, 0x07, 0x00, 0x00, 
    0x93, 0x88, 0x07, 0x00, 0x13, 0x05, 0x07, 0x00, 
    0x73, 0x00, 0x00, 0x00, 0x93, 0x07, 0x05, 0x00, 
    0x83, 0x20, 0xc1, 0x00, 0x13, 0x01, 0x01, 0x01, 
    0x67, 0x80, 0x00, 0x00, 
  };




  for (uint64_t addr = Hamster::MemorySpace::get_page_start(0x10094); addr <= Hamster::MemorySpace::get_page_start(0x10094 + sizeof(prog)); addr += HAMSTER_PAGE_SIZE)
  {
    mem_space.allocate_page(addr);
  }

  mem_space.memcpy(0x10094, prog, sizeof(prog));
  Hamster::RV32Thread thread(mem_space);

  thread.set_start_addr(0x10108);

  while (true)
  {
    int ret = thread.tick();
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
      auto syscall = thread.get_syscall();
      if (syscall.syscall_num == 0)
      {
        printf("Thread exited with code %lu\n", syscall.arg1);
        break;
      }
      if (syscall.syscall_num == 1)
      {
        printf("getting 12345\n");
        // return 12345
        thread.set_syscall_ret(12345);
        continue;
      }
      else
      {
        printf("Unknown syscall %lu\n", syscall.syscall_num);
        continue;
      }
    }
  }
}
