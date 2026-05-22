
#define DEBUG_ESP_PORT Serial
#define JSON_DEBUG_LEVEL 0

#include "./HardwareSerial.h"
#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string_view>
#include <vector>

#include "../src/JSONParser.h"
#include "../src/JSONPrinter.h"
#include "./StreamString.h"

// RapidJSON
#include "./rapidjson/document.h"
#include "./rapidjson/stringbuffer.h"
#include "./rapidjson/writer.h"

#include "../examples/JSONParserTest/test.h"

// ---------------------------------------------------------------------------
struct MyStruct {
  float temp;
  uint64_t timestamp;
  uint32_t updated;

  size_t fromJSON(const char *json) { return JSON::parse(updated, json, "temp", temp, "timestamp", timestamp); }

  template <typename T> size_t toJSON(T &stream, bool updates = true) {
    size_t mask = updates ? updated : 0;
    return JSON::print(mask, stream, "temp", temp, "timestamp", timestamp);
  }
};

int main() {
  run_tests();
  testGeoJSONParsingBig(); // requires tests/canada.json

  return 0;
}
