#include <platform/platform.hpp>

#include <process/subleq_process.hpp>

#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#endif

void test_platform();
void test_memory();

int main()
{
# ifdef ARDUINO
  Serial.begin(115200);
  while (!Serial)
    ;
  if (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("SD Card failed or not present");
    while (1)
      ;
  }
# endif // ARDUINO

  Hamster::_log("Testing Platform...\n");
  test_platform();
  Hamster::_log("Done\n");

  Hamster::_log("Testing Memory...\n");
  test_memory();
  Hamster::_log("Done\n");

  Hamster::_log("Testing Process...\n");
  auto process = Hamster::ProcessHelper::make_process(1, Hamster::ProcessTypes::subleq);
  Hamster::MemorySpace &mem = Hamster::ProcessHelper::get_memory_space(process);
  mem.allocate_page(HAMSTER_PAGE_SIZE * 1);
  mem.allocate_page(0);
  *(uint64_t*)mem.get_page_data(0) = HAMSTER_PAGE_SIZE;

  uint64_t data64[HAMSTER_PAGE_SIZE] = {
    0x1, 0x0, HAMSTER_PAGE_SIZE,
    0x2, 1234, HAMSTER_PAGE_SIZE + 24,
    HAMSTER_PAGE_SIZE + 72, HAMSTER_PAGE_SIZE + 72, HAMSTER_PAGE_SIZE
  };

  uint8_t *data = (uint8_t*)data64;

  for (int i = 0; i < HAMSTER_PAGE_SIZE; ++i)
  {
    mem[i + HAMSTER_PAGE_SIZE] = data[i];
  }

  int ret;

  while ((ret = process.tick64()) == 0)
    ;

  Hamster::_log("\nDone\n");
}
