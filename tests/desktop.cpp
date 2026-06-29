
#define DEBUG_ESP_PORT Serial
#define JSON_DEBUG_LEVEL JSON_DEBUG_LEVEL_NONE

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
#include "../src/fast_float.h"

int main() {
  bool all_passed = run_tests();
#ifdef JSON_DEBUG_MEM
  std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
  std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
#endif
  return all_passed ? 0 : 1;
  // return 0;
  //test fast_float
  // const char* p = "123.456 xx";
  // double d;
  // auto r = fast_float::from_chars(p, p + strlen(p), d);

  // if (r.ec == std::errc()) {
  //   std::cout << "success" << std::endl;
  // } else {
  //   std::cout << "fail" << std::endl;
  // }
  
  // std::cout << d << std::endl;
  // std::cout << (r.ptr - p) << std::endl;
  //return 0;
}
