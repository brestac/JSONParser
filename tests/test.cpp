#include <cmath>
#include <fstream>
#include <iostream>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ArduinoCompat.h"
#include "FileStream.h"
#include "HardwareSerial.h"
#include "StreamString.h"

#include <JSONParser.h>
#include <JSONPrinter.h>
#include "structs.h"
#include "geojson.h"

#define GEOJSON_TEST_FILE_PATH "./generated.geojson"
#define GEOJSON_BIG_FILE_PATH "./big.geojson"
#define GEOJSON_MEDIUM_FILE_PATH "./medium.geojson"
#define GEOJSON_SMALL_FILE_PATH "./small.geojson"
#define REQUIRE_FLOAT( a, b ) REQUIRE_THAT( a, WithinULP( b, 1 ) );
#define JSON_DEBUG_MEM

using Catch::Matchers::WithinULP;
// ---------------------------------------------------------------------------
//  generate_geojson
//
//  Génère un fichier GeoJSON d'environ target_kb kilo-octets dans LittleFS.
//
//  Paramètres :
//    path            – chemin dans LittleFS, ex: "/test.geojson"
//    target_kb       – taille cible en KB
//    rings           – nombre de rings par polygone (défaut 1)
//    points_per_ring – points par ring          (défaut 8)
//
//  Retourne la taille réelle écrite en octets, ou -1 en cas d'erreur.
// ---------------------------------------------------------------------------
static void _geojson_write_polygon(
    File& f, double cx, double cy, int rings, int points_per_ring ) {
  f.print( "{\"type\":\"Polygon\",\"coordinates\":[" );

  for ( int r = 0; r < rings; r++ ) {
    if ( r > 0 ) f.print( "," );

    double radius = 0.01 + ( rand() % 100 ) * 0.001;

    f.print( "[" );
    for ( int i = 0; i <= points_per_ring; i++ ) {
      if ( i > 0 ) f.print( "," );

      double angle = ( 2.0 * M_PI * i ) / points_per_ring;
      double lon = cx + radius * cos( angle );
      double lat = cy + radius * sin( angle );

      if ( lon > 180.0 ) lon = 180.0;
      if ( lon < -180.0 ) lon = -180.0;
      if ( lat > 90.0 ) lat = 90.0;
      if ( lat < -90.0 ) lat = -90.0;

      f.print( "[" );
      f.printf( "%f", lon );
      f.print( "," );
      f.printf( "%f", lat );
      f.print( "]" );
    }
    f.print( "]" );
  }

  f.print( "]}" );
}
long generate_geojson( const char* path,
                       size_t target_bytes,
                       int rings = 1,
                       int points_per_ring = 8 ) {

  // Supprimer le fichier existant éventuel
  if ( LittleFS.exists( path ) ) {
    LittleFS.remove( path );
    Serial.printf( "[GeoJSON] Deleted existing '%s'\n", path );
  }

  File f = LittleFS.open( path, "w" );

  if ( !f ) {
    Serial.printf( "[GeoJSON] Erreur: impossible d'ouvrir '%s' en écriture\n",
                   path );
    return -1;
  }

  randomSeed( static_cast<uint32_t>( micros() ) );

  f.print( "{\"type\":\"FeatureCollection\",\"features\":[" );

  size_t written = 0;
  int feat_count = 0;

  while ( true ) {
    // Coordonnées aléatoires
    double cx = -180.0 + ( random( 36000 ) ) * 0.01;
    double cy = -90.0 + ( random( 18000 ) ) * 0.01;

    // Estimation de la taille de la prochaine feature
    size_t est = 120 + (size_t)( rings * ( points_per_ring + 1 ) ) * 22;

    if ( written + est > target_bytes * 95 / 100 ) break;

    if ( feat_count > 0 ) f.print( "," );

    char name[32];
    snprintf( name, sizeof( name ), "feature_%d", feat_count );

    f.print( "{\"type\":\"Feature\","
             "\"properties\":{\"name\":\"" );
    f.print( name );
    f.print( "\",\"id\":" );
    f.printf( "%d", feat_count );
    f.print( "},\"geometry\":" );

    _geojson_write_polygon( f, cx, cy, rings, points_per_ring );

    f.print( "}" );

    written += est;
    feat_count++;

    // Laisser respirer le watchdog sur ESP8266/ESP32
    yield();
  }

  f.print( "]}" );
  f.flush();

  size_t actual = f.size();
  f.close();

  // std::printf( "[GeoJSON] %d features → %zu octets (%.1f KB) dans '%s'\n",
  //              feat_count,
  //              actual,
  //              (float)actual / 1024.0f,
  //              path );

  return (long)actual;
}

JSON::ParseResult
parse_with_callback_geojson_medium_from_stream( FeatureMultipoint& f,
                                                Stream* stream ) {
  DEBUG_PRINTF( "\nTEST GEOJSON MEDIUM PARSING WITH CALLBACK\n" );
  StreamCursorReader cursor( stream );
  // arrayIndex depth should be 0 for the geometry array, 1 for the rings, 2
  // for the coordinates, 3 for the lon/lat Ok got it. It is FeatureCollection
  // → feature → geometry → coordinates → rings → coordinates → lon/lat
  //                          0              1           2           3 4 5 6
  // the depth level is increased by 1 for each array level but not for the
  // object level
  strncpy( f.type, "Feature", sizeof( f.type ) );
  strncpy( f.properties.name, "France", sizeof( f.properties.name ) );
  strncpy( f.geometry.type, "LineString", sizeof( f.geometry.type ) );

  size_t count = 0;
  JSON::ParseResult pr = JSON::parse(
      cursor,
      [&f,
       &count]( const JSONKey& key, const JSONValue& value, JSON::SKIP& skip ) {
        // print the lat/lon of the first coordinate of any ring of the
        // first feature
        static float lon = 0.0f;

        if ( key != "coordinates" ) { return; }

        int16_t POLYGON_INDEX = key[-3];

        if ( POLYGON_INDEX > 0 ) {
          skip = JSON::SKIP::STOP;
          return;
        }

        int16_t COORDINATES_INDEX = key[-1];

        if ( COORDINATES_INDEX % 30 != 0 ) { return; }

        int16_t lon_lat_index = key.getArrayIndex();

        if ( lon_lat_index == 0 ) {
          lon = value;
        } else if ( lon_lat_index == 1 ) {
          // DEBUG_PRINTF("Ring#%d Coordinate: %f, %f\n",
          // key.getArrayIndex(2), coordinate[0], coordinate[1]);
          f.geometry.coordinates.push_back( { lon, value } );
          if ( count++ > 500 ) skip = JSON::SKIP::STOP;
          // if (key[-1] >= 10) skip = JSON::SKIP::END;
        }
      } );

  // Replace the last coordinate with the first one to close the LineString
  if ( f.geometry.coordinates.size() > 0 ) {
    f.geometry.coordinates.back() = f.geometry.coordinates.front();
  }

  return pr;
  // f.toJSON( Serial );
}

JSON::ParseResult
parse_with_callback_geojson_medium_from_file( FeatureMultipoint& feature ) {
  File file = LittleFS.open( "./medium.geojson", "r" );
  if ( !file ) {
    DEBUG_PRINTF( "Failed to open file for reading\n" );
    REQUIRE( false );
    return JSON::NO_RESULT;
  }

  return parse_with_callback_geojson_medium_from_stream( feature, &file );
}

static std::string read_file_to_string( const char* filename ) {
  std::ifstream file( filename );

  if ( !file.is_open() ) {
#ifdef __EXCEPTIONS
    throw std::runtime_error( "Could not open file" );
#else
    std::printf( "Could not open file %s\n", filename );
    return "";
#endif
  }
  std::stringstream buffer;
  buffer << file.rdbuf();

  return buffer.str();
}

TEST_CASE( "TEST CALLBACK", "" ) {
  char* ptr = (char*)"ptr";
  Personne p( "Bob", 40, 1.80f, "Paris", ptr, false, nullptr );

  const char json[] = "{\"ville\":\"Lyon\",\"taille\":2.1, \"age\":12.0, "
                     "\"flag\" : true, \"ptr\":null, "
                     "\"buffer\":\"AABBCCDD\",\"liste\":[\"a\", \"b\", \"c\"]}";

  int liste_idx = 0;
  JSON::ParseResult pr = JSON::parse(
      json,
      [&p, &liste_idx](
          const JSONKey& key, const JSONValue& value, JSON::SKIP& ) {
        if ( key == "ville" ) {
          p.ville = value;
        } else if ( key == "age" ) {
          p.age = value;
        } else if ( key == "taille" ) {
          p.taille = value;
        } else if ( key == "flag" ) {
          p.flag = value;
        } else if ( key == "ptr" ) {
          p.ptr = value;
        } else if ( key == "buffer" ) {
          value.copyTo( p.buffer );
        } else if ( key == "liste" ) {
          if ( liste_idx < 3 ) {
            auto sv = value.get<std::string_view>();
            strncpy( p.liste[liste_idx], sv.data(), sv.length() );
            ++liste_idx;
          }
        }
      } );

  REQUIRE( pr.error == 0 );
  REQUIRE( p.nom == std::string_view( "Bob" ) );
  REQUIRE( p.ville == std::string_view( "Lyon" ) );
  REQUIRE( p.age == 12 );
  REQUIRE_FLOAT( p.taille, 2.1f );
  REQUIRE( p.flag == true );
  REQUIRE( p.ptr == nullptr );
  REQUIRE( p.buffer[0] == 0xAA );
  REQUIRE( p.buffer[1] == 0xBB );
  REQUIRE( p.buffer[2] == 0xCC );
  REQUIRE( p.buffer[3] == 0xDD );
  REQUIRE( strcmp( p.liste[0], "a" ) == 0 );
  REQUIRE( strcmp( p.liste[1], "b" ) == 0 );
  REQUIRE( strcmp( p.liste[2], "c" ) == 0 );
}

TEST_CASE( "TEST ARRAY CALLBACK", "[.]" ) {

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
  REQUIRE( pr.error == 0 );
  REQUIRE( personnes[0].nom == std::string_view( "Bob" ) );
  REQUIRE( personnes[0].age == 0 );
  REQUIRE( personnes[1].nom == std::string_view( "Alice" ) );
  REQUIRE( personnes[1].age == 30 );
  REQUIRE( personnes[2].nom == std::string_view( "" ) );
  REQUIRE( personnes[2].age == 0 );
}

TEST_CASE( "TEST GEOJSON CALLBACK", "[geojson]" ) {
  FeatureMultipoint f;
  JSON::ParseResult pr = parse_with_callback_geojson_medium_from_file( f );

  REQUIRE( pr.error == 0 );
  REQUIRE( f.geometry.coordinates.size() > 0 );
  if ( f.geometry.coordinates.size() > 0 ) {
    REQUIRE_FLOAT( f.geometry.coordinates[0][0], 2.521800f );
    REQUIRE_FLOAT( f.geometry.coordinates[0][1], 51.087540f );
    REQUIRE_FLOAT( f.geometry.coordinates[1][0], 2.76296258f );
    REQUIRE_FLOAT( f.geometry.coordinates[1][1], 50.739384f );
  }
}

TEST_CASE( "TEST GEOJSON BIG", "[geojson][big][file][stream]" ) {
  FeatureCollectionPolygon fc;
  File f = LittleFS.open( GEOJSON_BIG_FILE_PATH, "r" );
  if ( !f ) {
    DEBUG_PRINTF( "Failed to open file for reading\n" );
    REQUIRE( false );
  }
  JSON::ParseResult pr = fc.fromJSON( &f );

  REQUIRE( pr.error == 0 );
  REQUIRE( fc.type == "FeatureCollection" );
  REQUIRE( fc.features.size() == 1 );
  REQUIRE( fc.features[0].geometry.coordinates.size() == 480 );
  REQUIRE( fc.features[0].geometry.coordinates[0].size() == 14 );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[0][0][0], -65.613616943f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[0][0][1], 43.420273437f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[479][32][0],
                 -68.977218628f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[479][32][1],
                 83.001663208f );
}

TEST_CASE( "TEST GEOJSON MEDIUM WITH SPACES", "[geojson][spaces][file][stream]" ) {
  FeatureCollectionMultiPolygon fc;
  File f = LittleFS.open( GEOJSON_MEDIUM_FILE_PATH, "r" );
  if ( !f ) {
    DEBUG_PRINTF( "Failed to open file for reading\n" );
    REQUIRE( false );
  }
  JSON::ParseResult pr = fc.fromJSON( &f );

  REQUIRE( pr.error == 0 );
}

TEST_CASE( "TEST GEOJSON PARSING BIG", "[geojson][big][buffer]" ) {
  FeatureCollectionPolygon fc;
  std::string json = read_file_to_string( GEOJSON_BIG_FILE_PATH );
  JSON::ParseResult pr = fc.fromJSON( json );

  REQUIRE( pr.error == 0 );
  REQUIRE( fc.type == "FeatureCollection" );
  REQUIRE( fc.features.size() == 1 );
  REQUIRE( fc.features[0].geometry.coordinates.size() == 480 );
  REQUIRE( fc.features[0].geometry.coordinates[0].size() == 14 );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[0][0][0], -65.613616943f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[0][0][1], 43.420273437f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[479][32][0],
                 -68.977218628f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[479][32][1],
                 83.001663208f );
}

TEST_CASE( "TEST GEOJSON PARSING MEDIUM", "[geojson][medium][buffer]" ) {
  FeatureCollectionMultiPolygon fc;
  std::string json = read_file_to_string( GEOJSON_MEDIUM_FILE_PATH );
  JSON::ParseResult pr = fc.fromJSON( json );

  REQUIRE( pr.error == 0 );
}

TEST_CASE( "TEST GEOJSON PARSING SMALL", "[geojson][small][buffer]" ) {

  const char* json = "{\"type\":\"FeatureCollection\",\"features\":["
                     "{\"type\":\"Feature\",\"properties\":{\"name\":"
                     "\"Canada\"},\"geometry\":"
                     "{\"type\":\"Polygon\",\"coordinates\":"
                     "[[[-140.99778,41.675105],[-140.99778,83.110903],"
                     "[-52.648098,83.110903],[-52.631163,41.675105],"
                     "[-140.99778,41.675105]],[[-140.99778,41.675105],"
                     "[-140.99778,83.110903],[-52.648098,83.110903],"
                     "[-52.631163,41.675105],[-140.99778,41.675105]],"
                     "[[-140.99778,41.675105],[-140.99778,83.110903],"
                     "[-52.648098,83.110903],[-52.631163,41.675105],"
                     "[-140.99778,41.675105]]]}}]}";

  FeatureCollectionPolygon fc;
  StreamString stream( json );
  JSON::ParseResult pr = fc.fromJSON( stream );

  REQUIRE( pr.error == 0 );
  REQUIRE( fc.type == "FeatureCollection" );
  REQUIRE( fc.features.size() == 1 );

  if ( fc.features.size() >= 1 ) {
    REQUIRE( strcmp( fc.features[0].type, "Feature" ) == 0 );
    REQUIRE( strcmp( fc.features[0].properties.name, "Canada" ) == 0 );
    REQUIRE( strcmp( fc.features[0].geometry.type, "Polygon" ) == 0 );
    REQUIRE( fc.features[0].geometry.coordinates.size() == 3 );

    if ( fc.features[0].geometry.coordinates.size() >= 2 ) {
      REQUIRE( fc.features[0].geometry.coordinates[0].size() == 5 );
      REQUIRE( fc.features[0].geometry.coordinates[1].size() == 5 );
      // Spot-REQUIRE first coordinate of ring[0]: [-140.99778, 41.675105]
      REQUIRE( fc.features[0].geometry.coordinates[0][0][0] ==
               -140.997787476f );
      REQUIRE( fc.features[0].geometry.coordinates[0][0][1] == 41.675106049f );
    }
  }
}

TEST_CASE( "TEST GEOJSON PARSING SUBSET", "" ) {
  File f = LittleFS.open( "./big.geojson", "r" );
  if ( !f ) {
    DEBUG_PRINTF( "Failed to open file for reading\n" );
    return;
  }

  FeatureCollectionSansGeometry fc;
  JSON::ParseResult pr = fc.fromJSON( &f );

  REQUIRE( pr.error == 0 );
  REQUIRE( fc.type == "FeatureCollection" );

  // fc.toJSON( Serial, false );
}

TEST_CASE( "TEST GEOJSON BIG", "[geojson][big][buffer][partial]" ) {
  std::string json = read_file_to_string( GEOJSON_BIG_FILE_PATH );
  FeatureCollectionLimited10Rings fc;
  JSON::ParseResult pr = fc.fromJSON( json );

  REQUIRE( pr.error == 0 );
  REQUIRE( fc.type == "FeatureCollection" );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[0][0][0], -65.613617f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[0][0][1], 43.420273f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[4][0][0], -61.199997f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[4][0][1], 45.558327f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[9][0][0], -64.039719f );
  REQUIRE_FLOAT( fc.features[0].geometry.coordinates[9][0][1], 46.743324f );
}

TEST_CASE( "TEST MULTIDIMENSIONAL ARRAY", "" ) {
  // std::vector<std::vector<std::vector<float>>> coordinates = { { { 0.0f,
  // 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f }  }  };
  float coordinates[1][3][2] = {
      { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } } };

  // using BaseContainerType = typename
  // container_info<decltype(coordinates)>::base_container_t; uint8_t path[3]
  // = { 0, 0, 0 }; BaseContainerType& base_container =
  // get_element_at_path<1>(coordinates, path);
  // JSON_DEBUG_TYPES("base_container %s\n", base_container);
  const char json[] = "{\"coordinates\":[[[1.0,2.0],[3.0,4.0],[5.0,6.0]]]}";
  JSON::ParseResult pr = JSON::parse( 0, json, "coordinates", coordinates );
  REQUIRE( pr.error == 0 );
  REQUIRE_FLOAT( coordinates[0][0][0], 1.0f );
  REQUIRE_FLOAT( coordinates[0][1][1], 4.0f );
  REQUIRE_FLOAT( coordinates[0][2][0], 5.0f );
  REQUIRE_FLOAT( coordinates[0][2][1], 6.0f );
}

TEST_CASE("Test characters after closing brace", "") {
  char key[16] = { 0 };
  const char json[] = "{\"key\":\"value\"}abc";
  JSON::ParseResult pr = JSON::parse( 0, json, "key", key );
  REQUIRE( pr.error == 0 );
  REQUIRE( strcmp( key, "value" ) == 0 );  
}

TEST_CASE( "TEST EMBEDDED OBJECT", "" ) {
  Parent parent;
  const char* json =
      "{\"nom\":\"Bob\",\"child\":{\"prenom\":\"Alice\",\"age\":8},"
      "\"child2\":{\"prenom\":\"Maxime\",\"age\":5},\"age\":40}";
  JSON::ParseResult pr = parent.fromJSON( json, strlen( json ) );
  REQUIRE( pr.error == 0 );
  REQUIRE( parent.nom == std::string_view( "Bob" ) );
  ;
  REQUIRE( parent.age == 40 );
  REQUIRE( parent.child.prenom == std::string_view( "Alice" ) );
  REQUIRE( parent.child2.age == 5 );
}

TEST_CASE( "TEST EMBEDDED OBJECT FROM STREAM", "" ) {
  Child child;
  Parent parent;
  parent.child = child;
  const char* json = "{\"nom\":\"Bob\",\"child\":{\"prenom\":\"Alice\","
                     "\"age\":8},\"age\":40}";
  StreamString stream( json );
  JSON::ParseResult pr = parent.fromJSON( stream );
  REQUIRE( pr.error == 0 );
  REQUIRE( parent.nom == std::string_view( "Bob" ) );
  ;
  REQUIRE( parent.age == 40 );
  REQUIRE( parent.child.prenom == std::string_view( "Alice" ) );
}

TEST_CASE( "TEST PARSING & FILL", "" ) {

  char* ptr = (char*)"ptr";
  Personne enfant( "", 10, 1.50f, "Lyon", ptr, false, nullptr );
  Personne p( "Bob", 40, 1.80f, "Paris", ptr, false, &enfant );

  const char* json =
      "{"
      "\"ville\":\"Lyon\","
      "\"age\":45.0, "
      "\"taille\":1.82, "
      "\"flag\":false, "
      "\"ptr\":null, "
      "\"buffer\":[170, 171, 172, 173, 174], "
      "\"liste\":[\"a\", \"b\", \"c\"], "
      "\"listef\":[1.0, 2.0, 3.0, 4.0, 5.0], "
      "\"enfant\":{\"nom\":\"Alice\",\"age\":8}, "
      "\"unknown\":1,"
      "\"enfants\":[{\"nom\":\"Alice\",\"age\":8,\"taille\":1.5},"
      "{\"nom\":\"Bob\",\"age\":10,\"taille\":1.6}],"
      "\"coordinates\":[[1.0,2.0],[3.0,4.0],[5.0,6.0],[7.0,8.0]],"
      "\"coordinates2\":[[1.0,2.0],[3.0,4.0],[5.0,6.0],[7.0,8.0]]"
      "}";

  JSON::ParseResult result = p.fromJSON( json, strlen(json) );

  REQUIRE( result.error == 0 );
  REQUIRE( p.ville == std::string_view( "Lyon" ) );
  REQUIRE( p.age == 45 );
  REQUIRE_FLOAT( p.taille, 1.82f );
  REQUIRE( p.flag == false );
  REQUIRE( p.ptr == nullptr );
  REQUIRE( p.buffer[0] == 170 );
  REQUIRE( p.buffer[1] == 171 );
  REQUIRE( p.buffer[2] == 172 );
  REQUIRE( p.buffer[3] == 173 );
  REQUIRE( strcmp( p.liste[0], "a" ) == 0 );
  REQUIRE( strcmp( p.liste[1], "b" ) == 0 );
  REQUIRE( strcmp( p.liste[2], "c" ) == 0 );
  REQUIRE_FLOAT( p.listef[0], 1.0f );
  REQUIRE_FLOAT( p.listef[1], 2.0f );
  REQUIRE_FLOAT( p.listef[2], 3.0f );
  REQUIRE_FLOAT( p.listef[3], 4.0f );
  REQUIRE_FLOAT( p.listef[4], 5.0f );
  REQUIRE( p.enfant != nullptr );
  if ( p.enfant ) {
    REQUIRE( p.enfant->nom == std::string_view( "Alice" ) );
    REQUIRE( p.enfant->age == 8 );
  }
  REQUIRE( p.enfants.size() == 2 );
  if ( p.enfants.size() >= 2 ) {
    REQUIRE( p.enfants[0].nom == std::string_view( "Alice" ) );
    REQUIRE_FLOAT( p.enfants[0].taille, 1.5f );
    REQUIRE( p.enfants[1].nom == std::string_view( "Bob" ) );
    REQUIRE_FLOAT( p.enfants[1].taille, 1.6f );
  }
}

// ----------------------------------------------------------------
// test_parse_top_level_array
// ----------------------------------------------------------------

TEST_CASE( "TEST ARRAY PARSING", "" ) {
  Personne personnes[3];

  const char* json = "[{\"nom\":\"Bob\",\"age\":40},{\"nom\":\"Alice\",\"age\":"
                     "30},{\"nom\":\"Roger\",\"age\":64}]";

  JSON::ParseResult r = JSON::parse( json, personnes );

  REQUIRE( r.error == 0 );
  REQUIRE( personnes[0].nom == std::string_view( "Bob" ) );
  REQUIRE( personnes[0].age == 40 );
  REQUIRE( personnes[1].nom == std::string_view( "Alice" ) );
  REQUIRE( personnes[1].age == 30 );
  REQUIRE( personnes[2].nom == std::string_view( "Roger" ) );
  REQUIRE( personnes[2].age == 64 );
}

// ----------------------------------------------------------------
// test_parse_indexed_keys
// ----------------------------------------------------------------

TEST_CASE( "TEST INDEXED PARSING", "" ) {
  std::string_view nom;
  int age;

  const char* json = "{ \"nom\":\"Bob\", \"age\":40, \"ville\":\"Paris\" }";
  uint32_t mask = 0;
  JSON::ParseResult pr = JSON::parse( mask, json, "nom[0]", nom, "age[1]", age );

  REQUIRE( pr.error == 0 );
  REQUIRE( nom == std::string_view( "Bob" ) );
  REQUIRE( age == 40 );
  REQUIRE( mask == 3 );

  Personne p;
  pr = p.fromJSON( json );
  REQUIRE( pr.error == 0 );
  REQUIRE( p.updated == ( 1 << 0 | 1 << 1 | 1 << 3 ) );
}

TEST_CASE( "TEST ARRAY OVERFLOW", "" ) {
  IntegralArray s;
  const char* json = "{\"hex\":[1,2,3,4,5], \"unknown\":1}";
  JSON::ParseResult pr = s.fromJSON( json, strlen(json) );

  REQUIRE( pr.error == 0 );
  REQUIRE( s.hex[0] == 1 );
  REQUIRE( s.hex[1] == 2 );
  REQUIRE( s.hex[2] == 3 );
  REQUIRE( s.hex[3] == 4 );
}

TEST_CASE( "TEST ARRAY OVERFLOW", "[array->array]" ) {
  STDArrayArray s;
  const char* json = "{\"array\":[[1,2],[3,4],[5,6]], \"unknown\":1}";
  JSON::ParseResult pr = s.fromJSON( json, strlen(json) );

  REQUIRE( pr.error == 0 );
  REQUIRE( s.array[0][0] == 1 );
  REQUIRE( s.array[1][0] == 3 );
  // s.toJSON( Serial );
}

TEST_CASE( "TEST ARRAY OVERFLOW", "[vector->array->array]" ) {
  VectorArrayArray s;
  const char* json = "{\"vector\":[[[1,2],[3,4],[5,6]], "
                     "[[1,2],[3,4],[5,6]]], \"unknown\":1}";
  JSON::ParseResult pr = s.fromJSON( json, strlen(json) );
  REQUIRE( pr.error == 0 );
  REQUIRE( s.vector[0][0][0] == 1 );
  REQUIRE( s.vector[1][1][0] == 3 );
}

// ----------------------------------------------------------------
// Test 1 – fromJSON via char buffer
// ----------------------------------------------------------------

TEST_CASE( "fromJSON via char buffer", "" ) {

  const char* json = "{\"id\":42,\"name\":\"abc\",\"temperature\":23.5,"
                     "\"active\":true, \"num\":[1,2,3]}";

  Sensor s;
  JSON::ParseResult result = s.fromJSON( json, strlen(json) );

  REQUIRE( result.error == 0 );
  REQUIRE( s.id == 42 );
  REQUIRE( strcmp( s.name, "abc" ) == 0 );
  REQUIRE( s.active == true );
  REQUIRE( s.num[0] == 1 );
  REQUIRE( s.num[1] == 2 );
  REQUIRE( s.num[2] == 3 );
  REQUIRE_FLOAT( s.temperature, 23.5f );
}
// ----------------------------------------------------------------
// Test 1 – fromJSON via StreamString
// ----------------------------------------------------------------

TEST_CASE( "fromJSON via StreamString", "" ) {

  const char* json = "{\"id\":42,\"temperature\":23.5,\"active\":true}";

  StreamString stream( json );

  Sensor s;
  s.id = 0;
  s.temperature = 0.0f;
  s.active = false;
  JSON::ParseResult result = s.fromJSON( stream );

  REQUIRE( result.error == 0 );
  REQUIRE( s.id == 42 );
  REQUIRE( s.active == true );
  REQUIRE_FLOAT( s.temperature, 23.5f );

  // s.toJSON(Serial);
}

// ----------------------------------------------------------------
// Test 2 – fromJSON partial update
// ----------------------------------------------------------------

TEST_CASE( "partial fromJSON via StreamString", "" ) {

  Sensor s;
  s.id = 99;
  s.temperature = 10.0f;
  s.active = false;

  // Only update "active"
  const char* json = "{\"temperature\":20}";
  StreamString stream( json );
  JSON::ParseResult result = s.fromJSON( stream );

  REQUIRE( result.error == 0 );
  REQUIRE( s.id == 99 );
  REQUIRE( s.temperature == 20 );
}

TEST_CASE( "parse JSON subset", "" ) {
  SensorMin s;
  const char* json = "{\"temp\":20, \"unrelated_1\":1, \"active\":false, "
                     "\"id\":42, \"unrelated_2\":1, \"unrelated_3\":1}";
  JSON::ParseResult result = s.fromJSON( json, strlen(json) );
  REQUIRE( result.error == 0 );
  REQUIRE( s.id == 42 );
  REQUIRE( s.active == false );
  REQUIRE( s.temp == 20.0f );
  REQUIRE( result.nParsed == 4 );
  REQUIRE( result.nMatched == 3 );
  REQUIRE( result.nConverted == 3 );
  REQUIRE( result.nUpdated == 2 );
}

TEST_CASE( "parse embedded object subset", "" ) {
  Parent p; // Parent can decode the keys nom, age, child. Child decodes the
            // keys nom, prenom, age
  const char* json = "{\"nom\":\"Bob\",\"child\":{\"nom\":\"Legendre\", "
                     "\"prenom\":\"Alice\",\"age\":8, \"unknown\": "
                     "{\"key_1\":0, \"key_2\":[1,2,3]}},\"age\":40}";
  JSON::ParseResult result = p.fromJSON( json, strlen(json) );
  REQUIRE( result.error == 0 );
  REQUIRE( result.nParsed == 3 );
  REQUIRE( result.nMatched == 3 );
  REQUIRE( p.age == 40 );
}

TEST_CASE( "parse empty object", "" ) {
  Parent p;
  const char* json = "{\"nom\":\"Bob\",\"child\":{},\"age\":40}";
  JSON::ParseResult result = p.fromJSON( json, strlen(json) );
  REQUIRE( result.error == 0 );
}

TEST_CASE( "key allowed characters", "" ) {
  int value = 0;
  const char *json = "{\"$kEy_\":1}";
  JSON::ParseResult result = JSON::parse(0, json, "$kEy_", value );
  REQUIRE( result.error == 0 );
  REQUIRE( value == 1 );
}

TEST_CASE( "parse JSON subset 2", "" ) {
  Parent p; // Parent can decode the keys nom, age, child, child2. Child
            // decodes the keys nom, prenom, age
  const char* json =
      "{\"nom\":\"Bob\",\"age\":40,\"child\":{\"nom\":\"Legendre\", "
      "\"prenom\":\"Alice\",\"age\":8} , \"child2\":{}, \"unknown\": "
      "{\"key_1\":0, \"key_2\":[1,2,3]}}";
  JSON::ParseResult result = p.fromJSON( json, strlen(json) );
  REQUIRE( result.error == 0 );
  REQUIRE( result.nParsed == 4 );
  REQUIRE( result.nMatched == 4 );
  REQUIRE( p.age == 40 );
}

// ----------------------------------------------------------------
// PRINTING TESTS
// ----------------------------------------------------------------

TEST_CASE( "print to buffer", "" ) {
  Sensor s;
  s.id = 7;
  s.temperature = 36;
  s.active = true;

  char buf[256] = {};
  size_t written = s.toJSON( buf );
  std::string expected = "{\"id\":7,\"active\":true,\"name\":\"\","
                         "\"temperature\":36,\"num\":[1,2,3]}";
  REQUIRE( std::string( buf ) == expected );
  REQUIRE( written == expected.length() );
}

// ----------------------------------------------------------------
// Test 5 – toJSON via StreamString
// ----------------------------------------------------------------

TEST_CASE( "toJSON via StreamString", "" ) {

  Sensor s;
  s.id = 7;
  s.temperature = 36.55f;
  s.active = true;
  strncpy( s.name,
           "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
           sizeof( s.name ) );

  // Use a StreamString as the backing stream;
  StreamString stream;

  size_t written = s.toJSON( stream );
  REQUIRE( written > 0 );
}

// ----------------------------------------------------------------
// Test 6 – toJSON Serial
// ----------------------------------------------------------------

TEST_CASE( "toJSON print to stdout", "" ) {
  Sensor s;
  s.id = 7;
  s.temperature = 36.6f;
  s.active = true;

  //  s.toJSON( Serial );
}

// TEST_CASE("--- Test: print vector property to stdout ---", "" ) {

//   VectorInt t;
//   t.numbers.push_back(1);
//   t.numbers.push_back(2);
//   t.numbers.push_back(3);

//   t.toJSON(Serial);
//   DEBUG_PRINTF("\n");
// }

// TEST_CASE("--- Test: print std::array property to stdout ---", "" )
// { Ring r; r.coordinates.push_back({1, 2}); r.coordinates.push_back({3, 4});
//   r.coordinates.push_back({5, 6});

//   r.toJSON(Serial);
//   DEBUG_PRINTF("\n");
// }

// TEST_CASE("--- Test: print std::vector property to stdout ---", ""
// ) { Geometry g; g.coordinates.push_back({{1, 2}, {3, 4}, {5, 6}, {7, 8}});
//   g.coordinates.push_back({{9, 10}, {11, 12}, {13, 14}, {15, 16}});
//   g.toJSON(Serial);
//   DEBUG_PRINTF("\n");
// }

// TEST_CASE("--- Test: print geojson feature to stdout ---", "" ) {
//   Feature f;
//   strncpy(f.type, "Feature", sizeof(f.type));
//   strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
//   strncpy(f.geometry.type, "Polygon", sizeof(f.geometry.type));
//   f.geometry.coordinates.push_back({{1, 2}, {3, 4}, {5, 6}, {7, 8}});
//   f.toJSON(Serial);
//   DEBUG_PRINTF("\n");
// }

// TEST_CASE("--- Test: print geojson feature to stdout ---", "" ) {
//   FeatureSansGeometry f;
//   strncpy(f.type, "Feature", sizeof(f.type));
//   strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
//   f.toJSON(Serial);
//   DEBUG_PRINTF("\n");
// }

// TEST_CASE("--- Test: print geojson to stdout ---", "" ) {
//   FeatureCollection fc;
//   fc.type = "FeatureCollection";
//   Feature f;
//   strncpy(f.type, "Feature", sizeof(f.type));
//   strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
//   strncpy(f.geometry.type, "Polygon", sizeof(f.geometry.type));
//   f.geometry.coordinates.push_back({{1, 2}, {3, 4}, {5, 6}, {7, 8}});
//   fc.features.push_back(f);
//   fc.toJSON(Serial);
//   DEBUG_PRINTF("\n");
// }

TEST_CASE( "print geojson to stdout", "" ) {
  FeatureCollectionPolygon fc;
  fc.type = "FeatureCollection";
  FeaturePolygon f;
  strncpy( f.type, "Feature", sizeof( f.type ) );
  strncpy( f.properties.name, "feature_0", sizeof( f.properties.name ) );
  strncpy( f.geometry.type, "Polygon", sizeof( f.geometry.type ) );
  f.geometry.coordinates.push_back(
      { { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } } );
  fc.features.push_back( f );

  char buf[1024] = { 0 };
  fc.toJSON( buf );
  const char* expected =
      "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
      "\"properties\":{\"name\":\"feature_0\"},\"geometry\":{\"type\":"
      "\"Polygon\",\"coordinates\":[[[1,2],[3,4],[5,6],[7,8]]]}}]}";
  REQUIRE( strcmp( buf, expected ) == 0 );
}

TEST_CASE( "toJSON StreamString", "" ) {
  StreamString stream;
  CharArrayTest s;
  s.name = "name";
  s.numbers[0] = 12;
  s.numbers[1] = 10000;
  strncpy( s.names[0], "a", 32 );
  strncpy( s.names[1], "b", 32 );
  s.toJSON( stream );
  std::string expected =
      "{\"name\":\"name\",\"names\":[\"a\",\"b\",\"\"],\"numbers\":[12,10000]}";
  REQUIRE( std::string( stream.c_str() ) == expected );
}

TEST_CASE( "toJSON StreamString with HEX uint8_t array", "" ) {
  StreamString stream;
  IntegralArray s;
  s.hex[0] = 0xAA;
  s.hex[1] = 0xBB;
  s.hex[2] = 0xCC;
  s.hex[3] = 0xDD;
  JSON::PRINT_BUFFER_AS_HEX = true;
  s.toJSON( stream );
  const char* expected = "{\"hex\":\"AABBCCDD\"}";
  JSON::PRINT_BUFFER_AS_HEX = false;
  REQUIRE( strcmp( stream.c_str(), expected ) == 0 );
}

// ----------------------------------------------------------------
// Test 7 – roundtrip: parse then re-serialize
// ----------------------------------------------------------------

TEST_CASE( "roundtrip parse → serialize", "" ) {

  Sensor original;
  original.id = 72;
  original.temperature = 19.8f;
  original.active = true;

  // Serialize original to a char buffer via PointerCursorWriter
  char buf[256] = { 0 };
  [[maybe_unused]] size_t len = original.toJSON( buf );
  // Serial.printf( "toJSON '%.*s'\n", (int)len, buf );

  StreamString stream( buf );
  Sensor copy;
  JSON::ParseResult result = copy.fromJSON( stream );

  REQUIRE( result.error == 0 );
  REQUIRE( copy.id == 72 );
  REQUIRE( copy.active == true );
  REQUIRE_FLOAT( copy.temperature, 19.8f );
}

// TEST_CASE( "Test: serialize to file", "" ) {

//   const char* filename = "./sensor.json";

//   Sensor s1;
//   s1.id = 8;
//   s1.temperature = 36.6f;

//   // delete file if it exists
//   if ( LittleFS.exists( filename ) ) { LittleFS.remove( filename ); }
//   // write sensor to file
//   File file_w = LittleFS.open( filename, "w" );
//   if ( !file_w ) {
//     DEBUG_PRINTF( "Failed to open file for writing\n" );
//     REQUIRE( false );
//     return;
//   }
//   s1.toJSON( file_w, false );
//   file_w.close();
//   // read file back
//   File file_r = LittleFS.open( filename, "r" );
//   if ( !file_r ) {
//     DEBUG_PRINTF( "Failed to open file for reading\n" );
//     REQUIRE( false );
//     return;
//   }

//   DEBUG_PRINTF( "FILE CONTENT ----\n" );
//   while ( file_r.available() > 0 ) {
//     DEBUG_PRINTF( "%c", file_r.read() );
//   }

//   file_r.seek( 0 );

//   Sensor s2;
//   JSON::ParseResult result = s2.fromJSON( file_r );
//   file_r.close();

//   REQUIRE( result.error == 0 );
//   REQUIRE( s2.id == 8 );
//   REQUIRE( s2.temperature == 36.6f );

//   LittleFS.remove( filename );
// }

// ----------------------------------------------------------------
// Test – BigStruct round-trip parse
// ----------------------------------------------------------------

TEST_CASE( "Test: BigStruct parse ", "" ) {

  const char* json = "{"
                     "\"f01\":true,"
                     "\"f02\":-12,"
                     "\"f03\":1000,"
                     "\"f04\":123456,"
                     "\"f05\":4000000000,"
                     "\"f06\":200,"
                     "\"f07\":50000,"
                     "\"f08\":3000000000,"
                     "\"f09\":1.5,"
                     "\"f10\":2.71828,"
                     "\"f11\":\"hello\","
                     "\"f12\":\"world_string\","
                     "\"f13\":\"view_one\","
                     "\"f14\":\"view_two\","
                     "\"f15\":false,"
                     "\"f33\":\"unknown_key\","
                     "\"f16\":-999,"
                     "\"f17\":3.14,"
                     "\"f18\":1.41421,"
                     "\"f19\":[1,2,3,4],"
                     "\"f20\":[100,200,300,400],"
                     "\"f21\":[1000,2000,3000,4000],"
                     "\"f22\":[1.1,2.2,3.3,4.4],"
                     "\"f23\":[-1,-2,-3,-4],"
                     "\"f24\":42,"
                     "\"f25\":-500,"
                     "\"f26\":255,"
                     "\"f27\":65000,"
                     "\"f28\":true,"
                     "\"f29\":0.12345,"
                     "\"f30\":\"short\","
                     "\"f31\":\"last_view\","
                     "\"f32\":77"
                     "}";

  BigStruct b;
  JSON::ParseResult r = b.fromJSON( json, strlen(json) );

  REQUIRE( r.error == 0 );
  REQUIRE( b.f01 == true );
  REQUIRE( b.f02 == -12 );
  REQUIRE( b.f03 == 1000 );
  REQUIRE( b.f04 == 123456 );
  REQUIRE( b.f05 == 4000000000LL );
  REQUIRE( b.f06 == 200 );
  REQUIRE( b.f07 == 50000 );
  REQUIRE( b.f08 == 3000000000U );
  REQUIRE_FLOAT( b.f09, 1.5f );
  REQUIRE_FLOAT( (float)b.f10, 2.71828f );
  REQUIRE( strcmp( b.f11, "hello" ) == 0 );
  REQUIRE( strcmp( b.f12, "world_string" ) == 0 );
  REQUIRE( b.f13 == std::string_view( "view_one" ) );
  REQUIRE( b.f14 == std::string_view( "view_two" ) );
  REQUIRE( b.f15 == false );
  REQUIRE( b.f16 == -999 );
  REQUIRE_FLOAT( b.f17, 3.14f );
  REQUIRE_FLOAT( (float)b.f18, 1.41421f );
  REQUIRE( b.f19[0] == 1 );
  REQUIRE( b.f19[1] == 2 );
  REQUIRE( b.f19[2] == 3 );
  REQUIRE( b.f19[3] == 4 );
  REQUIRE( b.f20[0] == 100 );
  REQUIRE( b.f20[1] == 200 );
  REQUIRE( b.f20[2] == 300 );
  REQUIRE( b.f20[3] == 400 );
  REQUIRE( b.f21[0] == 1000 );
  REQUIRE( b.f21[1] == 2000 );
  REQUIRE( b.f21[2] == 3000 );
  REQUIRE( b.f21[3] == 4000 );
  REQUIRE_FLOAT( b.f22[0], 1.1f );
  REQUIRE( b.f23[0] == -1 );
  REQUIRE( b.f23[1] == -2 );
  REQUIRE( b.f23[2] == -3 );
  REQUIRE( b.f23[3] == -4 );
  REQUIRE( b.f24 == 42 );
  REQUIRE( b.f25 == -500 );
  REQUIRE( b.f26 == 255 );
  REQUIRE( b.f27 == 65000 );
  REQUIRE( b.f28 == true );
  REQUIRE_FLOAT( (float)b.f29, 0.12345f );
  REQUIRE( strcmp( b.f30, "short" ) == 0 );
  REQUIRE( b.f31 == std::string_view( "last_view" ) );
  REQUIRE( b.f32 == 77 );
}

TEST_CASE( "parse escape sequence", "" ) {
  const char* json =
      "{\"prenom\":\"Jean dit \\\"Jeannot\\\" ou Jo\", \"nom\":\"Legendre\"}";
  Child p;
  JSON::ParseResult r = p.fromJSON( json, strlen(json) );
  REQUIRE( r.error == 0 );
  REQUIRE( p.prenom == std::string_view( "Jean dit \"Jeannot\" ou Jo" ) );
  REQUIRE( p.nom == std::string_view( "Legendre" ) );
}

TEST_CASE( "parse escape sequence with stream", "" ) {
  const char* json =
      "{\"prenom\":\"Jean dit \\\"Jeannot\\\" ou Jo\", \"nom\":\"Legendre\"}";
  StreamString stream( json );
  Child p;
  JSON::ParseResult r = p.fromJSON( stream );
  REQUIRE( r.error == 0 );
  REQUIRE( p.prenom == std::string_view( "Jean dit \"Jeannot\" ou Jo" ) );
  REQUIRE( p.nom == std::string_view( "Legendre" ) );
}

TEST_CASE( "parse strings from buffer", "" ) {
  Child enfant;
  const char* json = "{\"nom\":\"Legendre\", \"prenom\":\"Jean\", \"age\":42}";
  JSON::ParseResult r = enfant.fromJSON( json, strlen(json) );
  REQUIRE( r.error == 0 );
  REQUIRE( enfant.nom == std::string_view( "Legendre" ) );
  REQUIRE( enfant.prenom == std::string_view( "Jean" ) );
}

TEST_CASE( "parse strings from stream", "" ) {
  Child enfant;
  const char* json = "{\"nom\":\"Legendre\", \"prenom\":\"Jean\", \"age\":42}";
  StreamString stream( json );
  JSON::ParseResult r = enfant.fromJSON( stream );
  REQUIRE( r.error == 0 );
  REQUIRE( enfant.nom == std::string_view( "Legendre" ) );
  REQUIRE( enfant.prenom == std::string_view( "Jean" ) );
}
