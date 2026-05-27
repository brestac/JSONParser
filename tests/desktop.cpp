
#define DEBUG_ESP_PORT Serial
#define JSON_DEBUG_LEVEL 0

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

// RapidJSON
#include "../include/rapidjson/document.h"
#include "../include/rapidjson/stringbuffer.h"
#include "../include/rapidjson/writer.h"

#include "../examples/JSONParserTest/test.h"

uint32_t hash_32(const char *str, size_t len) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; ++i) {
    hash ^= static_cast<uint32_t>(str[i]);
    hash *= 16777619u;
  }
  return hash;
}

void test_hash32_performance() {
  const char *str = "hello world";
  size_t len = strlen(str);
  unsigned long long start = now();
  size_t count = 1000000000;

  for (int i = 0; i < count; i++) {
    hash_32(str, len);
  }
  unsigned long long end = now();
  std::printf("hash32 took %llu us\n", end - start);
}
int main() {
  run_tests();
  // test_parse_embedded_object();
  // testGeoJSONParsingBig();

  return 0;
}
