
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

#include "structs.h"
#include "geojson.h"
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

// static std::string_view name = "unknown";
// static int age = 0U;
// float height = 0.0f;


// struct Personne {
//   std::string_view name;
//   int age;

//   JSON_DECODER_IMPL( name, age );
// };

// std::string_view Personne::name = "unknown";
// int Personne::age = 0U;

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

int main() {

    
  // Personne p;
  // p.fromJSON( "{ \"name\":\"Bob\", \"age\":40}" );
  // std::printf("mask= %d name= %.*s\n", p.updated, (int)p.name.length(), p.name.data());

  // p.fromJSON( "{ \"name\":\"Alice\", \"age\":30}" );
  // std::printf("mask= %d name= %.*s\n", p.updated, (int)p.name.length(), p.name.data());


    // test_dispatch_table("name_1", name_1, "name_2", name_2, "age", age, "height", height);

    //JSON::parse(0, "{\"name\":\"roger\"}", "name", name);
    // constexpr auto dispatch_table = CREATE_DISPATCH_TABLE(name, age);
    // static_assert(dispatch_table.entries.size() == 2, "dispatch_table size is not 2");
    // JSON::_parse_impl<true, void>(0, "{\"name\":\"roger\"}", dispatch_table);
  
    // std::cout << "name:" << name << std::endl;
  Personne personnes[3];

  // age of personnes[0] is Infinity — not a valid number, stays 0
  const char json[] = "[{\"nom\":\"Bob\",\"age\":Infinity},{\"nom\":\"Alice\","
                     "\"age\":30},{\"nom\":\"Roger\",\"age\":64}]";

  JSON::ParseResult pr = JSON::parse(
      json,
      [&personnes](
          const JSONKey& key, const JSONValue& value, JSON::SKIP& skip ) {
        int arrayIndex = key.getArrayIndex();
        if ( arrayIndex < 0 || arrayIndex > 2 ) return;

        switch ( key ) {
          case "nom"_hash:
            personnes[arrayIndex].nom = value;
            break;
          case "age"_hash:
            personnes[arrayIndex].age = value;
            break;
          default:
            break;
        }
        if ( arrayIndex == 1 && key == "age" ) skip = JSON::SKIP::STOP;
      } );

    pr.print();
  // FeatureCollection<3> fc;
  // File input = LittleFS.open( GEOJSON_MEDIUM_FILE_PATH, "r" );
  // // //std::string input = read_file_to_string( GEOJSON_SMALL_FILE_PATH );
  // JSON::ParseResult r = fc.fromJSON( input );
  // r.print();
  // std::printf( "error: %d %s\n", r.error, errorToString( r.error ) );
  // std::printf("DISPATCH TABLE DURATION TOTAL: %lu\n", JSON::TIME_PROFILER);
// #ifdef JSON_DEBUG_MEM
//   std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
//   std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
// #endif

//   std::printf("TOTAL ITERATIONS: %lu\n", JSON::GLOBAL_ITERATIONS);
  return 0;
}
