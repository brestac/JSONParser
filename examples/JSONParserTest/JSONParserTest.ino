#define JSON_DEBUG_LEVEL 1

#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"


void setup() {
  delay(500);
  Serial.begin(115200);
  delay(500);

  run_tests();
}

void loop() {
    delay(10);
}
