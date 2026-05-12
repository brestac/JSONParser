#define JSON_DEBUG_LEVEL 0

#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "test.h"

void setup() {
    delay(500);
    Serial.begin(115200);
    delay(1000);

    run_tests();
}

void loop() {
    delay(10);
}
