#define JSON_DEBUG_LEVEL 0
#define JSON_DEBUG_MEM 1

#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"
#include "./get_stream.h"

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  LittleFS.begin();

  //run_tests();
  Serial.printf("sizeof FeatureMultipoint: %u\n", sizeof(FeatureMultipoint));
  Serial.printf("sizeof GeometryMultipoint: %u\n", sizeof(GeometryMultipoint));

  get_stream("http://192.168.1.2:10000/medium.geojson", [](WiFiClient* stream){
    Serial.printf("Stack before: %u\n", ESP.getFreeContStack());
    JSON::GLOBAL_CONTEXT_STACK_SIZE = ESP.getFreeContStack();
    test_parse_with_callback_geojson_medium_from_stream(stream);
    Serial.printf("Stack watermark: %u\n", ESP.getFreeContStack());
#ifdef JSON_DEBUG_MEM
    Serial.printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
#endif
  });

  // get_stream("http://192.168.1.2:10000/big.geojson", [](WiFiClient* stream){
  //   test_parse_geojson_big_with_limited_geometry_from_stream(stream);
  // });

  // constexpr size_t N = 60;
  // FeatureCollectionLimited<1, N, 1> fc;
  // JSON::ParseResult pr = fc.fromJSON(stream);

  // FeatureCollectionLimited<1, 1, N> fc2;
  // fc2.type = "FeatureCollection";
  // strncpy(fc2.features[0].type, "Feature", 32);
  // strncpy(fc2.features[0].properties.name, "Canada", 32);
  // strncpy(fc2.features[0].geometry.type, "Polygon", 32);

  // // loop the first coordinate of every ring of geometry of feature 0 of fc

  // for(size_t i = 0; i < N - 1; i++) {
  //   fc2.features[0].geometry.coordinates[0][i][0] = fc.features[0].geometry.coordinates[i][0][0];
  //   fc2.features[0].geometry.coordinates[0][i][1] = fc.features[0].geometry.coordinates[i][0][1];
  //   if (i % 64 == 0) yield();
  // }

  // fc2.features[0].geometry.coordinates[0][N - 1][0] = fc.features[0].geometry.coordinates[0][0][0];
  // fc2.features[0].geometry.coordinates[0][N - 1][1] = fc.features[0].geometry.coordinates[0][0][1];

  // fc2.toJSON(Serial);
}

void loop() {
  delay(10);
}
