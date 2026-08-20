//#define TEST_ARDUINO_JSON 1
#define ENABLE_ARGS_CHECK 0

#ifdef TEST_ARDUINO_JSON
#include <ArduinoJson.h>
#else
#include "src/JSONParser.h"
struct Simple : JSONObject {
  bool flag = false;
  JSON_DECODER_IMPL(flag);
};
#endif

void setup() {
  Serial.begin(115200);
  delay(500);

  const char* json = "{\"flag\":true}";
#ifdef TEST_ARDUINO_JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
#else
  Simple s;
  s.fromJSON(json);
  Serial.println("");
  Serial.printf("flag=%s\n", s.flag ? "true" : "false");
#endif
  // Allocate the JSON document

}

void loop() {
  // not used in this example
}

// ------------------------------------- //
// BUILD CONSOLE OUTPUT FOR EMPTY SKETCH //
// ------------------------------------- //
/*
 Variables and constants in RAM (global, static), used 28008 / 80192 bytes (34%)
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
╚══ IROM     231620   code in flash  */

// ------------------------------------- //
// BUILD CONSOLE OUTPUT FOR THIS SKETCH  //
// ------------------------------------- //
/*
. Variables and constants in RAM (global, static), used 28332 / 80192 bytes (35%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1516     initialized variables
╠══ RODATA   976      constants       
╚══ BSS      25840    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59747 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     26979    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 239300 / 1048576 bytes (22%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     239300   code in flash   
*/
/*
commit c67cc2d72ed2a34e5417900ec7de9fef666ceaff
. Variables and constants in RAM (global, static), used 28340 / 80192 bytes (35%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1496     initialized variables
╠══ RODATA   996      constants       
╚══ BSS      25848    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59771 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27003    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 239300 / 1048576 bytes (22%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     239300   code in flash
*/
// ------------------------------------- //
// ARDUINOJSON LIBRARY                   //
// ------------------------------------- //
/*
. Variables and constants in RAM (global, static), used 28336 / 80192 bytes (35%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1500     initialized variables
╠══ RODATA   1164     constants       
╚══ BSS      25672    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59875 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27107    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 240628 / 1048576 bytes (22%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     240628   code in flash 
*/
