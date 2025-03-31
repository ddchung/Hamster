#include <Arduino.h>
#include <platform/platform.hpp>
#include <SD.h>

void test_platform();
void test_memory();


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Card failed, or not present.");
    while (1);
  }
  Serial.println("Hello, world!");
  
  Serial.println("Testing platform...");
  test_platform();
  Serial.println("Platform test complete.");

  Serial.println("Testing memory...");
  test_memory();
  Serial.println("Memory test complete.");
}

void loop() {
  // put your main code here, to run repeatedly:
}
