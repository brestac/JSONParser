
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

int main() {
  run_tests();
  std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
  std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
  return 0;
}
