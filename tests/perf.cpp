// #include <catch2/benchmark/catch_benchmark_all.hpp>
// #include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <vector>

// This parser
#include <JSONParser.h>
#include <JSONPrinter.h>
// simd json
#include <simdjson.h>
// nohlmann json
#include <nlohmann/json.hpp>
// ArduinoJson
#include <ArduinoJson.h>
// RapidJSON
//#include <rapidjson/rapidjson.h>

#include "ArduinoCompat.h"
#include "FileStream.h"
#include "HardwareSerial.h"
#include "StreamString.h"
#include "globals.h"
#include "structs.h"

#define GEOJSON_GENERATED_FILE_PATH "./generated.geojson"
#define GEOJSON_BIG_FILE_PATH "./big.geojson"
#define GEOJSON_MEDIUM_FILE_PATH "./medium.geojson"
#define GEOJSON_SMALL_FILE_PATH "./small.geojson"
#define JSON_DEBUG_LEVEL 0
// GEOJSON_SMALL_FILE_PATH, GEOJSON_MEDIUM_FILE_PATH, GEOJSON_BIG_FILE_PATH
std::array<std::string, 3> FILE_PATHS = {
    GEOJSON_SMALL_FILE_PATH, GEOJSON_MEDIUM_FILE_PATH, GEOJSON_BIG_FILE_PATH };
/*
char *read_file(const char *filename) {
  FILE *file = fopen(filename, "r");

  if (!file) {
    DEBUG_PRINTF("ERROR: Could not open %s\n", filename);
    return nullptr;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)malloc(size + 1);
  [[maybe_unused]] size_t s = fread(buffer, 1, size, file);

  if (!buffer) {
    DEBUG_PRINTF("ERROR: Could not allocate buffer\n");
    fclose(file);
    return nullptr;
  }

  buffer[size] = '\0';
  fclose(file);

  return buffer;
}
*/
std::string read_file_to_string( const char* filename ) {
  std::ifstream file( filename );
  if ( !file.is_open() ) { throw std::runtime_error( "Could not open file" ); }
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

TEST_CASE( "Test string buffer", "[buffer]" ) {
  static uint8_t n = 0;

  auto path = GENERATE( as<std::string>{},
                        GEOJSON_SMALL_FILE_PATH,
                        GEOJSON_MEDIUM_FILE_PATH,
                        GEOJSON_BIG_FILE_PATH );

  std::string json = read_file_to_string( path.c_str() );
  current_size = json.size();

  std::printf( "#%d Testing file: %s with size %zu Bytes\n",
               n,
               path.c_str(),
               current_size );

  if ( n == 1 ) {
    BENCHMARK_ADVANCED("ce parser")(Catch::Benchmark::Chronometer meter) {
      FeatureCollection<3> fc;
      const char *json_cstr = json.c_str();
      meter.measure([&fc, &json_cstr] { return fc.fromJSON( json_cstr ); });
    };
  } else {
    BENCHMARK_ADVANCED( "ce parser" )(Catch::Benchmark::Chronometer meter) {
      FeatureCollection<2> fc;
      const char *json_cstr = json.c_str();
      meter.measure([&fc, &json_cstr] { return fc.fromJSON( json_cstr ); });
    };
  }
  
  BENCHMARK_ADVANCED("ArduinoJson")(Catch::Benchmark::Chronometer meter) {
    JsonDocument doc;
    meter.measure([&doc, &json] { return deserializeJson( doc, json ); });
  };

  BENCHMARK_ADVANCED("simd json")(Catch::Benchmark::Chronometer meter) {
    simdjson::ondemand::parser parser;
    simdjson::padded_string json_padded( json );

    meter.measure([&parser, &json_padded] { return parser.iterate( json_padded ); });
  };

  BENCHMARK("nlohmann json") {
    return nlohmann::json::parse( json );
  };

  n++;
}

TEST_CASE( "Test stream reader", "[stream][file]" ) {
  static uint8_t n = 0;

  auto path = GENERATE(
      as<std::string>{}, GEOJSON_SMALL_FILE_PATH, GEOJSON_BIG_FILE_PATH );

  File file = LittleFS.open( path.c_str(), "r" );
  if ( !file ) {
    DEBUG_PRINTF( "Failed to open file for reading\n" );
    REQUIRE( false );
  }

  current_size = file.size();
  std::printf( "#%d Testing file stream of size %zu\n", n, current_size );

  BENCHMARK( "ce parser" ) {
    FeatureCollection<2> fc;
    file.seek( 0 );
    JSON::ParseResult pr = fc.fromJSON( &file );
    REQUIRE( pr.error == 0 );
    return pr;
  };

  BENCHMARK( "arduino json" ) {
    JsonDocument doc;
    file.seek( 0 );
    DeserializationError error = deserializeJson( doc, file );
    REQUIRE( error == DeserializationError::Ok );
    return error;
  };

  file.close();
  n++;
}
