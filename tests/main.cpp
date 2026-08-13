
#define DEBUG_ESP_PORT Serial
#define JSON_DEBUG_LEVEL 0
#define DISABLE_ARGS_CHECK 1

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string_view>
#include <vector>

#include "../include/FileStream.h"
#include "../include/HardwareSerial.h"
#include "../include/StreamString.h"
#include "../src/JSONParser.h"
#include "../src/JSONPrinter.h"

#include "../examples/JSONParserTest/test.h"

int main() {
    const char *json = read_file("./medium.geojson");
    FeatureCollectionMultiPolygon fc;
    JSON::ParseResult pr = fc.fromJSON(json);
    std::printf( "pr.error=%d\n", pr.error );
    pr.print();
  // bool all_passed = run_tests();
  // test_parse_geojson_small();
  // test_parsing();
  // test_parse_multidimensional_array();
  // test_parse_indexed_keys();
  // test_parse_geojson_big_from_buffer<FeatureCollection>();
  // test_parse_geojson_big_from_file<FeatureCollection>();
  // test_parse_geojson_big_from_file<FeatureCollection>(true);
  // test_parse_geojson_big_from_file<FeatureCollection>(false);
#ifdef JSON_DEBUG_MEM
  std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
  std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
#endif
  // return all_passed ? 0 : 1;
  return 0;
}
