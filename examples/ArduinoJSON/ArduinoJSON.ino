#include <ArduinoJson.h>
#include "./get_stream.h"

JsonDocument doc;

void setup() {
  Serial.begin(115200);
  delay(500);

  get_stream("http://192.168.1.2:10000/100K.geojson", [](WiFiClient* stream) {
    Serial.printf("Stack before: %u\n", ESP.getFreeContStack());

    uint64_t start = micros();
    DeserializationError error = deserializeJson(doc, *stream);
    uint64_t elapsed = micros() - start;

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    Serial.printf("Parsing took %uus\n", elapsed);
    //serializeJson(doc, Serial);
    //Serial.println("");
    Serial.printf("Stack watermark: %u\n", ESP.getFreeContStack());
  });

// Results ESP-01

// Input size 500B
// Stack before: 2864
// Parsing took 4383us
// Stack watermark: 2152 -> -712

// Input size 1K
// Stack before: 2768
// Parsing took 10298us
// Stack watermark: 2152

// Input size 100K
// failed: NoMemory
}

//
void loop() {
  // not used in this example
}
// ------------------------------------- //
// BUILD CONSOLE OUTPUT FOR EMPTY SKETCH //
// ------------------------------------- //
/*
. Variables and constants in RAM (global, static), used 28008 / 80192 bytes (34%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1496     initialized variables
╠══ RODATA   920      constants       
╚══ BSS      25592    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59143 / 65536 bytes (90%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     26375    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 231620 / 1048576 bytes (22%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     231620   code in flash */

// ------------------------------------- //
// BUILD CONSOLE OUTPUT FOR THIS SKETCH  //
// ------------------------------------- //
/*
. Variables and constants in RAM (global, static), used 28896 / 80192 bytes (36%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1500     initialized variables
╠══ RODATA   1580     constants       
╚══ BSS      25816    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59887 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27119    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 261700 / 1048576 bytes (24%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     261700   code in flash  */
