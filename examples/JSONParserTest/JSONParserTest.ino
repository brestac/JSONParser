#define JSON_DEBUG_LEVEL 0

#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"

void setup() {
  Serial.begin(115200);
  delay(500);

  LittleFS.begin();

  Serial.println("");
  //   Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  //   Serial.printf("Free stack: %u\n", ESP.getFreeContStack());
  run_tests();
}

void loop() {
  delay(10);
}
