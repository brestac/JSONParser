
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

void test_string_pool_reuse() {
  std::printf("test_string_pool_reuse\n");
  struct Pool {};
  StaticString<Pool>::increase_pool_size(6);
  std::string_view sv1 = StaticString<Pool>::string_view("Hello", 5);
  std::printf("sv1='%.*s'\n", (int)sv1.size(), sv1.data());
  std::string_view sv2 = StaticString<Pool>::string_view("Hello", 5);
  std::printf("sv2='%.*s'\n", (int)sv2.size(), sv2.data());

  check(sv1.data() == sv2.data(), "sv1.data() == sv2.data()");
}

int main() {
  // run_tests();
  test_string_pool_reuse();
  test_parse_geojson_from_file();
  return 0;
}
