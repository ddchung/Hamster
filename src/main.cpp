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
  while (!SD.begin(BUILTIN_SDCARD))
  {
    Serial.println("No SD card found, retrying in 1 second...");
    delay(1000);
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
