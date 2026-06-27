#define JSON_DEBUG_LEVEL 0

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <StreamString.h>
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "./test.h"
#include "./get_stream.h"

static uint32_t free_heap = 0U;
static uint32_t free_stack = 0U;

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

  free_heap = ESP.getFreeHeap();
  free_stack = ESP.getFreeContStack();

  //run_tests();
  
  // get_stream("http://192.168.1.2:10000/fr.json", [](WiFiClient* stream){
  //   test_parse_with_callback_geojson_big_from_stream(stream);
  // });

  get_stream("http://192.168.1.2:10000/canada.json", [](WiFiClient* stream){
    test_parse_geojson_big_with_limited_geometry_from_stream(stream);
  });

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
  static bool printed = false;
  if (printed == false) {
    Serial.printf("Free heap: %u => %u\n", free_heap, ESP.getFreeHeap());
    Serial.printf("Free stack: %u => %u\n", free_stack, ESP.getFreeContStack());
    Serial.printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
    printed = true;
  }

  delay(10);
}
