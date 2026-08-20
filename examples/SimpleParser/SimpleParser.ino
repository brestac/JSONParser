
#include "src/JSONParser.h"
#include "./get_stream.h"

struct Properties : public JSONObject {
  char name[32] = { 0 };
  JSON_DECODER_IMPL(name);
};

struct Geometry : public JSONObject {
  using Coordinate = std::array<float, 2>;
  using Ring = std::array<Coordinate, 9>;
  char type[32] = { 0 };
  std::array<Ring, 1> coordinates;
  JSON_DECODER_IMPL(type, coordinates);
};

struct Feature : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  Geometry geometry;
  JSON_DECODER_IMPL(type, properties, geometry);
};

struct FeatureCollection : public JSONObject {
  std::string_view type = "";
  std::array<Feature, 1> features;
  JSON_DECODER_IMPL(type, features);
};

FeatureCollection fc;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  get_stream("http://192.168.1.2:10000/big.geojson", [](WiFiClient* stream){
    Serial.printf("Stack before: %u\n", ESP.getFreeContStack());
    
    uint64_t start = micros();
    JSON::ParseResult pr = fc.fromJSON(stream);
    uint64_t elapsed = micros() - start;

    if (pr.error != JSON::NO_ERROR) {
      Serial.printf("Json deserialization failed with error %s", errorToString(pr.error));
      return;
    }

    Serial.printf("Parsing took %uus\n", elapsed);
    Serial.printf("Stack watermark: %u\n", ESP.getFreeContStack());
  });
// Results ESP-01
// Input size 500B
// Stack before: 2544\n
// Parsing took 7080us
// Stack watermark: 1800 => -744

// Input size 1K
// Stack before: 2864
// Parsing took 12209us
// Stack watermark: 1800

// Input size 100K
// Stack before: 2672
// Parsing took 1099239us
// Stack watermark: 1800

// Input size 100K 01.08.26
// Stack before: 2816
// Parsing took 1036772us
// Stack watermark: 1976
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
/*. Variables and constants in RAM (global, static), used 43028 / 80192 bytes (53%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1536     initialized variables
╠══ RODATA   13660    constants       
╚══ BSS      27832    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59807 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27039    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 313936 / 1048576 bytes (29%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     313936   code in flash*/
/*
commit c67cc2d72ed2a34e5417900ec7de9fef666ceaff
. Variables and constants in RAM (global, static), used 29576 / 80192 bytes (36%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ DATA     1496     initialized variables
╠══ RODATA   1712     constants       
╚══ BSS      26368    zeroed variables
. Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR), used 59783 / 65536 bytes (91%)
║   SEGMENT  BYTES    DESCRIPTION
╠══ ICACHE   32768    reserved space for flash instruction cache
╚══ IRAM     27015    code in IRAM    
. Code in flash (default, ICACHE_FLASH_ATTR), used 265976 / 1048576 bytes (25%)
║   SEGMENT  BYTES    DESCRIPTION
╚══ IROM     265976   code in flash 
*/