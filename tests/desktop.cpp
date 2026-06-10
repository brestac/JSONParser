
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

// template<typename T>
// struct TypeId {
//     static size_t value;
// };

// template<typename T> size_t TypeId<T>::value = getId();

// struct A {};
// struct B {};
// struct C {};

// template<size_t N>
// struct Target {
//     Target() {
//         std::cout << "Target<T>::Target() " << N << std::endl;
//     }
// };
// template<typename T> static size_t getId();
// template<typename T> static bool add_clearer(size_t idx);
// static void clear_all();

template <typename T = void> struct Target {
  Target() = delete;
  ~Target() = delete;
  static size_t id;
  static size_t count;
  static std::array<void (*)(), 10> clearers;

  template <typename U = T> static void clearall();

  template <> static void clearall() {
    std::cout << "clear_all() count:" << count << std::endl;

    for (size_t i = 0; i < count; i++) {
      auto clearer = clearers[i];
      if (clearer)
        clearer();
    }
  }

  static size_t add_clearer() {
    std::cout << "Target<T>::add_clearer() " << Target<>::count << std::endl;
    if (id >= Target<>::clearers.size())
      return false;
    Target<>::clearers[Target<>::count] = clear;
    return Target<>::count++;
  }

  static void _do() { std::cout << "Target<T>::_do() " << id << std::endl; }

  static void clear() { std::cout << "Target<T>::clear() " << id << std::endl; }
};

using TargetManager = Target<>;
template <typename T> size_t Target<T>::id = Target<T>::add_clearer();
template <> size_t TargetManager::count = 0;
template <> std::array<void (*)(), 10> TargetManager::clearers = {nullptr};

int main() {
  run_tests();
  std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
  std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
  return 0;
}
