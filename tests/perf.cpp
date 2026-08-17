#define DISABLE_ARGS_CHECK
#define JSON_DEBUG_LEVEL 0
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
#include <rapidjson/document.h> // Inclusion du DOM RapidJSON
#include <rapidjson/writer.h>   // Inclusion du générateur JSON
#include <rapidjson/stringbuffer.h> // Inclusion du tampon de chaîne

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

// GEOJSON_SMALL_FILE_PATH, GEOJSON_MEDIUM_FILE_PATH, GEOJSON_BIG_FILE_PATH
std::array<std::string, 3> FILE_PATHS = {
    GEOJSON_SMALL_FILE_PATH, GEOJSON_MEDIUM_FILE_PATH, GEOJSON_BIG_FILE_PATH };

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

  BENCHMARK_ADVANCED("rapid json")(Catch::Benchmark::Chronometer meter) {
      std::string local_json = json; // Copie locale si nécessaire hors chrono
      rapidjson::Document doc;
  
      meter.measure([&local_json, &doc] {
          doc.Parse(local_json.c_str());
          return !doc.HasParseError(); // Retourne un booléen (autorisé par Catch2 pour empêcher l'optimisation)
      });
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


  //BENCHMARK_ADVANCED("ce parser")(Catch::Benchmark::Chronometer meter) {
  BENCHMARK("ce parser") {
    FeatureCollection<2> fc;
    file.seek( 0 );

      //meter.measure([&fc,&file] {
        JSON::ParseResult pr = fc.fromJSON( file );
        //REQUIRE( pr.error == 0 );
        return pr;
      //});
  };

  // BENCHMARK_ADVANCED("ArduinoJson")(Catch::Benchmark::Chronometer meter) {
    BENCHMARK("ArduinoJson") {
      JsonDocument doc;
      file.seek( 0 );
  
      //meter.measure([&doc,&file] {
        DeserializationError error = deserializeJson( doc, file );
        //REQUIRE( error == DeserializationError::Ok );
        return error;
      //});
  };

  file.close();
  n++;
}
