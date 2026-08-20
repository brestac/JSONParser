
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

#include "FileStream.h"
#include "HardwareSerial.h"
#include "StreamString.h"

#include <JSONParser.h>
#include <JSONPrinter.h>
#include "structs.h"
//#include "../examples/JSONParserTest/test.h"
#define GEOJSON_SMALL_FILE_PATH "./small.geojson"

std::string read_file_to_string( const char* filename ) {
  std::ifstream file( filename );
  if ( !file.is_open() ) { throw std::runtime_error( "Could not open file" ); }
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

int main() {
  FeatureCollection<2> fc;
  File input = LittleFS.open( GEOJSON_SMALL_FILE_PATH, "r" );
  // //std::string input = read_file_to_string( GEOJSON_SMALL_FILE_PATH );
  JSON::ParseResult r = fc.fromJSON( input );
  r.print();
  std::printf("Refill total duration %.2f%% read bytes %.2f%%\n", ((float)refill_duration / r.elapsed) * 100.0f, ((float)profile_read_bytes / r.elapsed) * 100.0f);
  // std::printf( "error: %d %s\n", r.error, errorToString( r.error ) );
  
#ifdef JSON_DEBUG_MEM
  std::printf("GLOBAL_STRING_POOL_SIZE=%zu\n", JSON::GLOBAL_STRING_POOL_SIZE);
  std::printf("MAX_GLOBAL_PARSER_SIZE=%zu\n", JSON::MAX_GLOBAL_PARSER_SIZE);
#endif

  return 0;
}
