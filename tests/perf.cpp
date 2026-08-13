#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>

#include <vector>
#include <algorithm>

#include "include/ArduinoCompat.h"
#include "include/FileStream.h"
#include "include/HardwareSerial.h"
#include "include/StreamString.h"

// This parser
#include "src/JSONParser.h"
#include "src/JSONPrinter.h"
#include "structs.h"

// simd json
#include <simdjson.h>
// nohlmann json
#include <nlohmann/json.hpp>
// RapidJSON
// #include <rapidjson/document.h>
// #include <rapidjson/stringbuffer.h>
// #include <rapidjson/writer.h>
// ArduinoJson
#include <ArduinoJson.h>

#define GEOJSON_GENERATED_FILE_PATH "./generated.geojson"
#define GEOJSON_BIG_FILE_PATH "./big.geojson"
#define GEOJSON_MEDIUM_FILE_PATH "./medium.geojson"
#define GEOJSON_SMALL_FILE_PATH "./small.geojson"
#define JSON_DEBUG_LEVEL 0
// GEOJSON_SMALL_FILE_PATH, GEOJSON_MEDIUM_FILE_PATH, GEOJSON_BIG_FILE_PATH
std::vector<std::string> FILE_PATHS = { GEOJSON_SMALL_FILE_PATH, GEOJSON_MEDIUM_FILE_PATH, GEOJSON_BIG_FILE_PATH };
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
  if ( !file.is_open() ) {
    throw std::runtime_error( "Could not open file" );
  }
  std::stringstream buffer;
  buffer << file.rdbuf();

  //file.close();
  return buffer.str();
}

TEST_CASE( "Parsing", "[performance]" ) {

  for ( auto& filename : FILE_PATHS ) {
    std::string json = read_file_to_string( filename.c_str() );
    REQUIRE( json.size() > 0 );

    char section_name[64] = { 0 };
    snprintf( section_name, sizeof( section_name ), "%zu Bytes", json.size());
    bool is_medium = filename.find( "medium" ) != std::string::npos;

    SECTION( section_name ) {

      BENCHMARK( "ce parser" ) {
        if (is_medium) {
          FeatureCollectionMultiPolygon fc;
          JSON::ParseResult r = fc.fromJSON( json.c_str() );
          REQUIRE( r.error == 0 );
          return r;
        } else {
          FeatureCollection fc;
          JSON::ParseResult r = fc.fromJSON( json.c_str() );
          REQUIRE( r.error == 0 );
          return r;
        }
      };

      BENCHMARK( "arduino json" ) {
        JsonDocument doc;
        DeserializationError error = deserializeJson( doc, json );
        REQUIRE( error == DeserializationError::Ok );
        return error;
      };

      BENCHMARK( "simd json" ) {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json_padded(json);

        auto doc_result = parser.iterate(json_padded);
        REQUIRE(doc_result.error() == simdjson::SUCCESS);
        return doc_result;
      };

      BENCHMARK( "nlohmann json" ) {
        nlohmann::json doc = nlohmann::json::parse(json);
        REQUIRE(doc.is_object());
        return doc;
      };

      // BENCHMARK( "rapid json" ) {
      //   rapidjson::Document doc;
      //   return doc.Parse( json );
      // };
      
    }
    //if (json != nullptr) free(json);
  }
}
