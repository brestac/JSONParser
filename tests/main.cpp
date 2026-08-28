
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
// include std::any
#include <any>

#include "FileStream.h"
#include "HardwareSerial.h"
#include "StreamString.h"

#include <JSONParser.h>
#include <JSONPrinter.h>

//#include "structs.h"
#include "../src/Reflection.h"

#define GEOJSON_SMALL_FILE_PATH "./small.geojson"
#define GEOJSON_MEDIUM_FILE_PATH "./medium.geojson"
#define GEOJSON_BIG_FILE_PATH "./big.geojson"

std::string read_file_to_string( const char* filename ) {
  std::ifstream file( filename );
  if ( !file.is_open() ) { throw std::runtime_error( "Could not open file" ); }
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

std::string_view name_1 = "roger";
std::string_view name_2 = "albert";
int age = 45U;
float height = 1.80f;

struct Personne : JSONObject {
  std::string_view com;
  int age;

  JSON_DECODER_IMPL( com, age );
};

template <typename... Args>
void test_dispatch_table(Args... args) {

  //constexpr auto dispatch_table_object = CREATE_DISPATCH_TABLE(name_1, height, name_2);
  //constexpr auto dispatch_table_api = create_dispatch_table("name_1", name_1, "name_2", name_2, "age", age, "height", height);

  auto dispatch_table_api = create_dispatch_table((args)...);
  static_assert(dispatch_table_api.size() == 4, "dispatch_table_api size is not 4");
  // static_assert(dispatch_table_object.size() == 3, "dispatch_table_object size is not 3");

  //  auto variant2 = find_entry(dispatch_table_object, "height");

  // std::visit([](auto&& arg) {
  //   using T = std::decay_t<decltype(arg)>;

  //   if constexpr (!std::is_same_v<T, std::monostate>) {
  //     std::printf("Found entry height: %s\n", typeid(arg.ref).name());
  //     if constexpr (std::is_same_v<remove_cv_ref_t<decltype(arg.ref)>, float>) {
  //       std::cout << "height:" << arg.ref << std::endl;
  //       arg.ref = 1.90f;
  //       std::cout << "height:" << arg.ref << std::endl;
  //     }
  //   }
  // }, variant2);

  //  std::cout << "height:" << height << std::endl;

}

// void print_byte(uint8_t byte) {
//   for (int i = 7; i >= 0; i--) {
//     std::cout << ((byte >> i) & 1);
//   }
// }

// void test(unsigned char c, unsigned char* array, size_t len) {
//   std::printf("%*c ",9U, c);
//   print_byte(c);
//   std::printf(" is_for_sure_not_in_set: %.*s => %d\n",(int)len, array, is_for_sure_not_in_set(c, array, len));
// }

// void test_my_dumb_stuff() {
//   unsigned char array[] = { 'f', 'p', 'n', 'v' };

//   uint8_t common_0 = 0XFF;
//   uint8_t common_1 = 0XFF;
//   get_common_bits_equal_to_0_1(array, sizeof(array), common_0, common_1);
//   std::printf("common_0: "); print_byte(common_0); std::cout << std::endl;
//   std::printf("common_1: "); print_byte(common_1); std::cout << std::endl;

//   uint8_t len = sizeof(array) / sizeof(uint8_t);
//   for (uint8_t i = 'a'; i < 'z'; i++) {
//     test(i, array, len);
//   }
// }

struct Element {
  size_t hash;

  Element(size_t hash) : hash(hash) {}
};

template <size_t N>
int find_lower_bound(const std::array<Element, N>& arr, size_t value) {
  auto it = std::lower_bound(arr.begin(), arr.end(), value, [](const Element& a, size_t b) {
    return a.hash < b;
  });

  if (it != arr.end() && it->hash == value) {
    return it->hash;
  }

  return -1;
}

int main() {

  // Test std::array find sorted with comparator

  std::array<Element, 1> arr = { 5 };
  std::sort(arr.begin(), arr.end(), [](const Element& a, const Element& b) {
    return a.hash < b.hash;
  });

  int result = find_lower_bound(arr, 5);
  std::cout << "Result: " << result << std::endl;

  int result2 = find_lower_bound(arr, 6);
  std::cout << "Result: " << result2 << std::endl;  

  
  // Personne p;
  // p.fromJSON( "{ \"com\":\"Bob\", \"age\":40}" );
  // std::printf("mask= %d\n", p.updated);

    // test_dispatch_table("name_1", name_1, "name_2", name_2, "age", age, "height", height);

    // JSON::parse(0, "{\"name\":\"roger\"}", "name", name_1);
  
    // std::cout << "name:" << name_1 << std::endl;
  
//   FeatureCollection<3> fc;
//   File input = LittleFS.open( GEOJSON_MEDIUM_FILE_PATH, "r" );
//   // //std::string input = read_file_to_string( GEOJSON_SMALL_FILE_PATH );
//   JSON::ParseResult r = fc.fromJSON( input );
//   r.print();
//   // std::printf( "error: %d %s\n", r.error, errorToString( r.error ) );
  
// #ifdef JSON_DEBUG_MEM
//   std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
//   std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
// #endif

//   std::printf("TOTAL ITERATIONS: %lu\n", JSON::GLOBAL_ITERATIONS);
  return 0;
}
