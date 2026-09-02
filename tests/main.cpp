
#define DEBUG_ESP_PORT Serial
#define JSON_DEBUG_LEVEL 2
#define DISABLE_ARGS_CHECK 0

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string_view>
#include <vector>
#include <any>
#include <optional>
#include <functional>


template <typename T>
using remove_cv_ref_t = std::remove_cv_t<std::remove_reference_t<T>>;

#include "FileStream.h"
#include "HardwareSerial.h"
#include "StreamString.h"

//#include "../src/Dispatch.h"
#include <JSONParser.h>
#include <JSONPrinter.h>

// #include "structs.h"
// #include "geojson.h"
//#include "../src/Reflection.h"

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

std::string_view name = "unknown";
int age = 0U;
// float height = 0.0f;

struct Personne {
  // static std::string_view name;
  // static int age;
  std::string_view name = "unknown";
  int age = 0;

  JSON_DECODER_IMPL( name, age );
};

// std::string_view Personne::name = "unknown";
// int Personne::age = 0U;


Personne p;

template <typename... Args>
void test_with_args(Args&& ...args) {
    //constexpr auto tuple = create_tuple_pair(std::tuple<>{} , std::forward<Args>(args)...);
    //static_assert(std::tuple_size<remove_cv_ref_t<decltype(tuple)>>::value == 2, "tuple size is not 2");
    dispatch_by_key(create_dispatch_tuple(std::forward<Args>(args)...), "name");
    std::cout << "Name: " << p.name << std::endl;
}

int main() {

    //test_with_args("name", p.name, "age", p.age);
    static auto tuple = create_dispatch_tuple("name", p.name, "age", p.age);
    static_assert(std::tuple_size<remove_cv_ref_t<decltype(tuple)>>::value == 2, "tuple size is not 2");
    dispatch_by_key(tuple, "name", [](auto& value, size_t idx) {
      std::cout << "Valeur: " << value << std::endl;
      if constexpr (std::is_same_v<decltype(value), std::string_view&>) {
        value = "test";
      } else if constexpr (std::is_same_v<decltype(value), int&>) {
        value = 42;
      }
    });
      
     std::cout << "Name: " << p.name << std::endl;
  
  Personne p;
  p.fromJSON( "{ \"name\":\"Bob\", \"age\":40}" );
  std::printf("mask= %d name= %.*s\n", p.updated, (int)p.name.length(), p.name.data());

  // p.fromJSON( "{ \"name\":\"Alice\", \"age\":30}" );
  // std::printf("mask= %d name= %.*s\n", p.updated, (int)p.name.length(), p.name.data());


    // constexpr auto dispatch_table = CREATE_DISPATCH_TABLE(name, age);
    // // static_assert(std::tuple_size<remove_cv_ref_t<decltype(dispatch_table)>>::value == 2, "dispatch_table size is not 2");
    
    // JSON::parse(0, "{\"name\":\"roger\"}", dispatch_table);
    // std::cout << "name:" << name << std::endl;
  
  // Personne personnes[3];
  // // age of personnes[0] is Infinity — not a valid number, stays 0
  // const char json[] = "[{\"nom\":\"Bob\",\"age\":Infinity},{\"nom\":\"Alice\","
  //                    "\"age\":30},{\"nom\":\"Roger\",\"age\":64}]";

  // JSON::ParseResult pr = JSON::parse(
  //     json,
  //     [&personnes](
  //         const JSONKey& key, const JSONValue& value, JSON::SKIP& skip ) {
  //       int arrayIndex = key.getArrayIndex();
  //       if ( arrayIndex < 0 || arrayIndex > 2 ) return;

  //       switch ( key ) {
  //         case "nom"_hash:
  //           personnes[arrayIndex].nom = value;
  //           break;
  //         case "age"_hash:
  //           personnes[arrayIndex].age = value;
  //           break;
  //         default:
  //           break;
  //       }
  //       if ( arrayIndex == 1 && key == "age" ) skip = JSON::SKIP::STOP;
  //     } );

  //   pr.print();
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
