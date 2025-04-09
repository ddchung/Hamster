#include <platform/platform.hpp>

#ifdef ARDUINO
#include <Arduino.h>
#include <SD.h>
#endif

void test_platform();
void test_memory();
void test_filesystem();

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

  Hamster::_log("Testing Filesystem...\n");
  test_filesystem();
  Hamster::_log("Done\n");
}
