#include "src/ParseResult.h"
#include <chrono>
#include <math.h>
#include <stdlib.h>

#ifdef ARDUINO
#include <LittleFS.h>
#define GEOJSON_TEST_FILE_PATH "/generated.geojson"
#else
#include "../../include/ArduinoCompat.h"
#include "../../include/FileStream.h"
#define GEOJSON_TEST_FILE_PATH "./generated.geojson"
#endif


using namespace std;
using namespace JSON;

static void _geojson_write_polygon(File &f, double cx, double cy, int rings,
                                   int points_per_ring) {
  f.print("{\"type\":\"Polygon\",\"coordinates\":[");

  for (int r = 0; r < rings; r++) {
    if (r > 0)
      f.print(",");

    double radius = 0.01 + (rand() % 100) * 0.001;

    f.print("[");
    for (int i = 0; i <= points_per_ring; i++) {
      if (i > 0)
        f.print(",");

      double angle = (2.0 * M_PI * i) / points_per_ring;
      double lon = cx + radius * cos(angle);
      double lat = cy + radius * sin(angle);

      if (lon > 180.0)
        lon = 180.0;
      if (lon < -180.0)
        lon = -180.0;
      if (lat > 90.0)
        lat = 90.0;
      if (lat < -90.0)
        lat = -90.0;

      f.print("[");
      f.printf("%f", lon);
      f.print(",");
      f.printf("%f", lat);
      f.print("]");
    }
    f.print("]");
  }

  f.print("]}");
}

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

long generate_geojson(const char *path, size_t target_bytes, int rings = 1,
                      int points_per_ring = 8) {

  // Supprimer le fichier existant éventuel
  if (LittleFS.exists(path)) {
    LittleFS.remove(path);
    Serial.printf("[GeoJSON] Deleted existing '%s'\n", path);
  }

  File f = LittleFS.open(path, "w");

  if (!f) {
    Serial.printf("[GeoJSON] Erreur: impossible d'ouvrir '%s' en écriture\n",
                  path);
    return -1;
  }

  randomSeed(micros());

  f.print("{\"type\":\"FeatureCollection\",\"features\":[");

  size_t written = 0;
  int feat_count = 0;

  while (true) {
    // Coordonnées aléatoires
    double cx = -180.0 + (random(36000)) * 0.01;
    double cy = -90.0 + (random(18000)) * 0.01;

    // Estimation de la taille de la prochaine feature
    size_t est = 120 + (size_t)(rings * (points_per_ring + 1)) * 22;

    if (written + est > target_bytes * 95 / 100)
      break;

    if (feat_count > 0)
      f.print(",");

    char name[32];
    snprintf(name, sizeof(name), "feature_%d", feat_count);

    f.print("{\"type\":\"Feature\","
            "\"properties\":{\"name\":\"");
    f.print(name);
    f.print("\",\"id\":");
    f.printf("%d", feat_count);
    f.print("},\"geometry\":");

    _geojson_write_polygon(f, cx, cy, rings, points_per_ring);

    f.print("}");

    written += est;
    feat_count++;

    // Laisser respirer le watchdog sur ESP8266/ESP32
    yield();
  }

  f.print("]}");
  f.flush();

  size_t actual = f.size();
  f.close();

  Serial.printf("[GeoJSON] %d features → %zu octets (%.1f KB) dans '%s'\n",
                feat_count, actual, (float)actual / 1024.0f, path);

  return (long)actual;
}

// ----------------------------------------------------------------
// Test infrastructure
// ----------------------------------------------------------------

static int passed = 0;
static int failed = 0;

template<typename... Args>
static void check(bool condition, std::string_view format, Args &&...args) {
  bool print_args = true;

  if (condition) {
    PRINTF_COLOR(COLOR_GREEN, "%*c[PASS] ", 2, ' ');
    ++passed;
    size_t pos = format.find(", was");
    if (pos == std::string_view::npos) pos = format.find(", should be");

    if (pos != std::string_view::npos) {
      format = format.substr(0, pos);
      print_args = false;
    }
  } else {
    PRINTF_COLOR(COLOR_RED, "%*c[FAIL] ", 2, ' ');
    ++failed;
  }

  if (sizeof...(Args) > 0 && print_args) {
    char _buf[512];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    snprintf(_buf, sizeof(_buf), format.data(), std::forward<Args>(args)...);
#pragma clang diagnostic pop
    DEBUG_PRINTF("%s\n", _buf);
  } else {
    DEBUG_PRINTF("%.*s\n", (int)format.length(), format.data());
  }
}

static bool near(float a, float b, float tol = 0.01f) {
  float d = a - b;
  return d > -tol && d < tol;
}

// ----------------------------------------------------------------
// Structs
// ----------------------------------------------------------------

struct BigStruct : public JSONObject {
  bool f01 = false;
  int8_t f02 = 0;
  int16_t f03 = 0;
  int32_t f04 = 0;
  int64_t f05 = 0;
  uint8_t f06 = 0;
  uint16_t f07 = 0;
  uint32_t f08 = 0;
  float f09 = 0.0f;
  double f10 = 0.0;
  char f11[32] = { 0 };
  char f12[64] = { 0 };
  std::string_view f13 = "";
  std::string_view f14 = "";
  bool f15 = false;
  int32_t f16 = 0;
  float f17 = 0.0f;
  double f18 = 0.0;
  uint8_t f19[4] = { 0 };
  uint16_t f20[4] = { 0 };
  uint32_t f21[4] = { 0 };
  float f22[4] = { 0.0f };
  int32_t f23[4] = { 0 };
  int8_t f24 = 0;
  int16_t f25 = 0;
  uint8_t f26 = 0;
  uint16_t f27 = 0;
  bool f28 = false;
  double f29 = 0.0;
  char f30[16] = { 0 };
  std::string_view f31 = "";
  int32_t f32 = 0;
  JSON_SERIALIZE_IMPL(f01, f02, f03, f04, f05, f06, f07, f08, f09, f10, f11,
                      f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22,
                      f23, f24, f25, f26, f27, f28, f29, f30, f31, f32);
};

struct Sensor : public JSONObject {
  int id = 0;
  float temperature = 1.0f;
  bool active = false;
  char name[64] = { 0 };
  uint8_t num[3] = { 1, 2, 3 };
  JSON_SERIALIZE_IMPL(id, active, name, temperature, num);
};

struct SensorMin : public JSONObject {
  int id = 0;
  uint8_t temp = 0;
  bool active = false;
  JSON_SERIALIZE_IMPL(id, active, temp);
};

struct Config : public JSONObject {
  int version = 0;
  float interval = 0.0f;
  JSON_SERIALIZE_IMPL(version, interval);
};

struct CharArrayTest : JSONObject {
  std::string_view name = "";
  char names[3][32] = { { '\0' } };
  uint32_t numbers[2] = { 0 };

  JSON_ENCODER_IMPL(name, names, numbers);
};

struct IntegralArray : JSONObject {
  uint8_t hex[4] = { 0 };

  JSON_SERIALIZE_IMPL(hex);
};

struct STDArrayArray : JSONObject {
  using STDArray = std::array<uint8_t, 2>;
  std::array<STDArray, 2 > array;

  JSON_SERIALIZE_IMPL(array);
};

struct VectorInt : JSONObject {
  std::vector<int> numbers;
  JSON_SERIALIZE_IMPL(numbers);
};

struct VectorArrayArray : JSONObject {
  using coord = std::array<uint8_t, 2>;
  using ring = std::array<coord, 2>;

  std::vector<ring> vector;

  JSON_SERIALIZE_IMPL(vector);
};

struct Child : public JSONObject {
  std::string_view nom = "";
  std::string_view prenom = "";
  uint8_t age = 0U;

  JSON_SERIALIZE_IMPL(nom, prenom, age);
};

struct Parent : public JSONObject {
  std::string_view nom = "";
  uint8_t age = 0U;
  Child child;

  JSON_SERIALIZE_IMPL(nom, age, child);
};

struct Personne : public JSONObject {
public:
  std::string_view nom = "";
  uint8_t age = 0U;
  float taille = 0.0F;
  std::string_view ville = "";
  char *ptr;
  bool flag = false;

  uint8_t buffer[4] = { 0 };
  char liste[3][32] = { { '\0' } };
  float listef[5] = { 0 };
  Personne *enfant;
  std::vector<Personne> enfants;
  float coordinates[4][2];
  vector<std::array<float, 2>> coordinates2;

  Personne()
    : JSONObject() {}
  Personne(std::string_view nom, int age, float taille, std::string_view ville,
           char *ptr, bool flag, Personne *enfant)
    : JSONObject(), nom(nom), age(age), taille(taille), ville(ville),
      ptr(ptr), flag(flag), enfant(enfant) {}

  JSON_SERIALIZE_IMPL(nom, age, taille, ville, ptr, flag, buffer, liste, listef,
                      enfant, enfants, coordinates);
};

struct Properties : public JSONObject {
  char name[32] = { 0 };
  JSON_SERIALIZE_IMPL(name);
};

struct Geometry : public JSONObject {
  using Coordinate = std::array<float, 2>;
  using Ring = std::vector<Coordinate>;
  char type[32] = { 0 };
  std::vector<Ring> coordinates;
  JSON_SERIALIZE_IMPL(type, coordinates);
};

template<size_t R, size_t C>
struct GeometryLimited : public JSONObject {
  using Coordinate = std::array<float, 2>;
  using Ring = std::array<Coordinate, C>;
  char type[32] = { 0 };
  std::array<Ring, R> coordinates = {};
  JSON_SERIALIZE_IMPL(type, coordinates);
};

struct GeometryMultipoint : public JSONObject {
  char type[32] = { 0 };
  std::vector<std::array<float, 2>> coordinates;
  JSON_SERIALIZE_IMPL(type, coordinates);
};

struct Feature : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  Geometry geometry;
  JSON_SERIALIZE_IMPL(type, properties, geometry);
};

struct FeatureSansGeometry : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  JSON_SERIALIZE_IMPL(type, properties);
};

template<size_t R, size_t C>
struct FeatureLimited : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  GeometryLimited<R, C> geometry;
  JSON_SERIALIZE_IMPL(type, properties, geometry);
};

struct FeatureMultipoint : public JSONObject {
  char type[32] = { 0 };
  Properties properties;
  GeometryMultipoint geometry;
  JSON_SERIALIZE_IMPL(type, properties, geometry);
};

struct FeatureCollection : public JSONObject {
  std::string_view type = "";
  std::vector<Feature> features;
  JSON_SERIALIZE_IMPL(type, features);
};

struct FeatureCollectionSansGeometry : public JSONObject {
  std::string_view type = "";
  std::vector<FeatureSansGeometry> features;
  JSON_SERIALIZE_IMPL(type, features);
};

template<size_t F, size_t R, size_t C>
struct FeatureCollectionLimited : public JSONObject {
  std::string_view type = "";
  std::array<FeatureLimited<R, C>, F> features;
  JSON_SERIALIZE_IMPL(type, features);
};

using FeatureCollectionLimited10Rings = FeatureCollectionLimited<1, 10, 1>;

// ----------------------------------------------------------------
// test_parse_callback
// ----------------------------------------------------------------

void test_parse_callback() {
  DEBUG_PRINTF("\nTEST CALLBACK\n");
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  char *ptr = (char *)"ptr";
  Personne p("Bob", 40, 1.80f, "Paris", ptr, false, nullptr);

  const char *json = "{\"ville\":\"Lyon\",\"taille\":2.1, \"age\":12.0, "
                     "\"flag\" : true, \"ptr\":null, "
                     "\"buffer\":\"AABBCCDD\",\"liste\":[\"a\", \"b\", \"c\"]}";

  int liste_idx = 0;
  JSON::ParseResult pr = JSON::parse(
    json, [&p, &liste_idx](const JSONKey &key, const JSONValue &value,
                           JSON::SKIP & /*stop*/) {
      if (key == "ville") {
        p.ville = value;
      } else if (key == "age") {
        p.age = value;
      } else if (key == "taille") {
        p.taille = value;
      } else if (key == "flag") {
        p.flag = value;
      } else if (key == "ptr") {
        p.ptr = value;
      } else if (key == "buffer") {
        value.copyTo(p.buffer);
      } else if (key == "liste") {
        if (liste_idx < 3) {
          auto sv = value.get<std::string_view>();
          strncpy(p.liste[liste_idx], sv.data(), sv.length());
          ++liste_idx;
        }
      }
    });

  check(pr.error == 0, "parse");
  check(p.nom == std::string_view("Bob"), "nom unchanged (Bob)");
  check(p.ville == std::string_view("Lyon"), "ville == Lyon");
  check(p.age == 12, "age == 12");
  check(near(p.taille, 2.1f), "taille ≈ 2.1");
  check(p.flag == true, "flag == true");
  check(p.ptr == nullptr, "ptr == nullptr (null)");
  check(p.buffer[0] == 0xAA && p.buffer[1] == 0xBB && p.buffer[2] == 0xCC && p.buffer[3] == 0xDD,
        "buffer == {AA BB CC DD}");
  check(strcmp(p.liste[0], "a") == 0, "liste[0] == 'a'");
  check(strcmp(p.liste[1], "b") == 0, "liste[1] == 'b'");
  check(strcmp(p.liste[2], "c") == 0, "liste[2] == 'c'");
}

// ----------------------------------------------------------------
// test_parse_array_callback
// ----------------------------------------------------------------

void test_parse_array_callback() {
  DEBUG_PRINTF("\nTEST ARRAY CALLBACK\n");
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  Personne personnes[3];

  // age of personnes[0] is Infinity — not a valid number, stays 0
  const char *json = "[{\"nom\":\"Bob\",\"age\":Infinity},{\"nom\":\"Alice\","
                     "\"age\":30},{\"nom\":\"Roger\",\"age\":64}]";

  JSON::ParseResult pr = JSON::parse(
      json, [&personnes](const JSONKey &key, const JSONValue &value, JSON::SKIP &skip) {
        uint16_t arrayIndex = key.getArrayIndex();
        if (arrayIndex >= 3 || arrayIndex < 0)
          return;

      switch (key) {
        case "nom"_hash:
          personnes[arrayIndex].nom = value;
          break;
        case "age"_hash:
          personnes[arrayIndex].age = value;
          break;
        default:
          break;
      }
      if (arrayIndex == 1 && key == "age")
        skip = JSON::SKIP::STOP;
    });
  check(pr.error == false, "parse");
  check(personnes[0].nom == std::string_view("Bob"), "personnes[0].nom == Bob");
  check(personnes[0].age == 0, "personnes[0].age == 0 (Infinity)");
  check(personnes[1].nom == std::string_view("Alice"),
        "personnes[1].nom == Alice");
  check(personnes[1].age == 30, "personnes[1].age == 30");
  check(personnes[2].nom == std::string_view(""),
        "personnes[2].nom unchanged (empty) (stopped)");
  check(personnes[2].age == 0, "personnes[2].age unchanged (0) (stopped)");
}

void test_parse_embedded_object() {
  DEBUG_PRINTF("\nTEST EMBEDDED OBJECT\n");
  Child child;
  Parent parent;
  parent.child = child;
  const char *json =
    "{\"nom\":\"Bob\",\"child\":{\"prenom\":\"Alice\",\"age\":8},\"age\":40}";
  JSON::ParseResult pr = parent.fromJSON(json);
  check(pr.error == 0, "parse");
  check(parent.nom == std::string_view("Bob"), "parent.nom == Bob, was %.*s",
        (int)parent.nom.length(), parent.nom.data());
  check(parent.age == 40, "parent.age == 40, was %d", parent.age);
  check(parent.child.prenom == std::string_view("Alice"),
        "parent.child.prenom == Alice, was %.*s",
        (int)parent.child.prenom.length(), parent.child.prenom.data());
}

void test_parse_embedded_object_from_stream() {
  DEBUG_PRINTF("\nTEST EMBEDDED OBJECT FROM STREAM\n");
  Child child;
  Parent parent;
  parent.child = child;
  const char *json =
    "{\"nom\":\"Bob\",\"child\":{\"prenom\":\"Alice\",\"age\":8},\"age\":40}";
  StreamString stream(json);
  JSON::ParseResult pr = parent.fromJSON(stream);
  check(pr.error == 0, "parse");
  check(parent.nom == std::string_view("Bob"), "parent.nom == Bob, was %.*s",
        (int)parent.nom.length(), parent.nom.data());
  check(parent.age == 40, "parent.age == 40, was %d", parent.age);
  check(parent.child.prenom == std::string_view("Alice"),
        "parent.child.prenom == Alice, was %.*s",
        (int)parent.child.prenom.length(), parent.child.prenom.data());
}

// ----------------------------------------------------------------
// test_parsing from const char *
// ----------------------------------------------------------------

void test_parsing() {
  JSON::PRINT_BUFFER_AS_HEX = false;
  DEBUG_PRINTF("\n\nTEST PARSING & FILL\n");
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  char *ptr = (char *)"ptr";
  Personne enfant("", 10, 1.50f, "Lyon", ptr, false, nullptr);
  Personne p("Bob", 40, 1.80f, "Paris", ptr, false, &enfant);

  const char *json =
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

  JSON::ParseResult result = p.fromJSON(json);

  check(result.error == 0, "parse ok, was %s:%s", errorToString(result.error), errorToString(result.parseError));
  check(p.ville == std::string_view("Lyon"), "ville == Lyon");
  check(p.age == 45, "age == 45");
  check(near(p.taille, 1.82f), "taille ≈ 1.82");
  check(p.flag == false, "flag == false");
  check(p.ptr == nullptr, "ptr == nullptr (null)");
  check(p.buffer[0] == 170 && p.buffer[1] == 171 && p.buffer[2] == 172 && p.buffer[3] == 173,
        "buffer == [170..173]");
  check(strcmp(p.liste[0], "a") == 0, "liste[0] == 'a'");
  check(strcmp(p.liste[1], "b") == 0, "liste[1] == 'b'");
  check(strcmp(p.liste[2], "c") == 0, "liste[2] == 'c'");
  check(near(p.listef[0], 1.0f), "listef[0] ≈ 1.0");
  check(near(p.listef[1], 2.0f), "listef[1] ≈ 2.0");
  check(near(p.listef[2], 3.0f), "listef[2] ≈ 3.0");
  check(near(p.listef[3], 4.0f), "listef[3] ≈ 4.0");
  check(near(p.listef[4], 5.0f), "listef[4] ≈ 5.0");
  check(p.enfant != nullptr, "enfant != nullptr");
  if (p.enfant) {
    check(p.enfant->nom == std::string_view("Alice"), "enfant.nom == Alice");
    check(p.enfant->age == 8, "enfant.age == 8");
  }
  check(p.enfants.size() == 2, "enfants.size() == 2");
  if (p.enfants.size() >= 2) {
    check(p.enfants[0].nom == std::string_view("Alice"),
          "enfants[0].nom == Alice");
    check(near(p.enfants[0].taille, 1.5f), "enfants[0].taille ≈ 1.5");
    check(p.enfants[1].nom == std::string_view("Bob"), "enfants[1].nom == Bob");
    check(near(p.enfants[1].taille, 1.6f), "enfants[1].taille ≈ 1.6");
  }
  check(near(p.coordinates[0][0], 1.0f) && near(p.coordinates[0][1], 2.0f),
        "coordinates[0] ≈ [1.0, 2.0]");
  check(near(p.coordinates[1][0], 3.0f) && near(p.coordinates[1][1], 4.0f),
        "coordinates[1] ≈ [3.0, 4.0]");
  check(near(p.coordinates[2][0], 5.0f) && near(p.coordinates[2][1], 6.0f),
        "coordinates[2] ≈ [5.0, 6.0]");
  check(near(p.coordinates[3][0], 7.0f) && near(p.coordinates[3][1], 8.0f),
        "coordinates[3] ≈ [7.0, 8.0]");
}

// ----------------------------------------------------------------
// test_parse_top_level_array
// ----------------------------------------------------------------

void test_parse_top_level_array() {
  DEBUG_PRINTF("\n\nTEST ARRAY PARSING\n");
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  Personne personnes[3];

  const char *json = "[{\"nom\":\"Bob\",\"age\":40},{\"nom\":\"Alice\",\"age\":"
                     "30},{\"nom\":\"Roger\",\"age\":64}]";
  uint32_t mask = 0;
  JSON::ParseResult r = JSON::parse(mask, json, personnes);

  check(r.error == 0, "parse");
  check(personnes[0].nom == std::string_view("Bob"), "personnes[0].nom == Bob");
  check(personnes[0].age == 40, "personnes[0].age == 40");
  check(personnes[1].nom == std::string_view("Alice"),
        "personnes[1].nom == Alice");
  check(personnes[1].age == 30, "personnes[1].age == 30");
  check(personnes[2].nom == std::string_view("Roger"),
        "personnes[2].nom == Roger");
  check(personnes[2].age == 64, "personnes[2].age == 64");
}

// ----------------------------------------------------------------
// test_parse_indexed_keys
// ----------------------------------------------------------------

void test_parse_indexed_keys() {
  DEBUG_PRINTF("\n\nTEST INDEXED PARSING\n");
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  std::string_view nom;
  int age;

  const char *json = "{ \"nom\":\"Bob\", \"age\":40, \"ville\":\"Paris\" }";
  uint32_t mask = 0;
  JSON::ParseResult pr = JSON::parse(mask, json, "nom[0]", nom, "age[1]", age);

  check(pr.error == 0, "parse");
  check(nom == std::string_view("Bob"), "nom == Bob");
  check(age == 40, "age == 40");
  check(mask == 3, "mask == 3 (bits 0 and 1 set), was %llu", mask);
}

void test_parse_geojson_small() {
  DEBUG_PRINTF("\n\nTEST GEOJSON PARSING SMALL\n");
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  const char *json =
    "{\"type\":\"FeatureCollection\",\"features\":["
    "{\"type\":\"Feature\",\"properties\":{\"name\":\"Canada\"},\"geometry\":"
    "{\"type\":\"Polygon\",\"coordinates\":"
    "[[[-140.99778,41.675105],[-140.99778,83.110903],"
    "[-52.648098,83.110903],[-52.631163,41.675105],"
    "[-140.99778,41.675105]],[[-140.99778,41.675105],"
    "[-140.99778,83.110903],[-52.648098,83.110903],"
    "[-52.631163,41.675105],[-140.99778,41.675105]],"
    "[[-140.99778,41.675105],[-140.99778,83.110903],"
    "[-52.648098,83.110903],[-52.631163,41.675105],"
    "[-140.99778,41.675105]]]}}]}";

  FeatureCollection fc;
  StreamString stream(json);
  JSON::ParseResult pr = fc.fromJSON(stream);

  check(pr.error == 0, "parse");
  check(fc.type == "FeatureCollection", "type == FeatureCollection, was %.*s",
        (int)fc.type.length(), fc.type.data());
  check(fc.features.size() == 1, "1 feature, was %u", fc.features.size());

  if (fc.features.size() >= 1) {
    check(strcmp(fc.features[0].type, "Feature") == 0,
          "feature.type == Feature, was %s", fc.features[0].type);
    check(strcmp(fc.features[0].properties.name, "Canada") == 0,
          "properties.name == Canada, was %s", fc.features[0].properties.name);
    check(strcmp(fc.features[0].geometry.type, "Polygon") == 0,
          "geometry.type == Polygon, was %s", fc.features[0].geometry.type);
    check(fc.features[0].geometry.coordinates.size() == 3, "3 rings, was %u",
          fc.features[0].geometry.coordinates.size());
    if (fc.features[0].geometry.coordinates.size() >= 2) {
      check(fc.features[0].geometry.coordinates[0].size() == 5,
            "ring[0] has 5 points, was %u",
            fc.features[0].geometry.coordinates[0].size());
      check(fc.features[0].geometry.coordinates[1].size() == 5,
            "ring[1] has 5 points, was %u",
            fc.features[0].geometry.coordinates[1].size());
      // Spot-check first coordinate of ring[0]: [-140.99778, 41.675105]
      check(near(fc.features[0].geometry.coordinates[0][0][0], -140.99778f,
                 0.001f),
            "ring[0][0].lon ≈ -140.998, was %f",
            fc.features[0].geometry.coordinates[0][0][0]);
      check(near(fc.features[0].geometry.coordinates[0][0][1], 41.675105f,
                 0.001f),
            "ring[0][0].lat ≈ 41.675, was %f",
            fc.features[0].geometry.coordinates[0][0][1]);
    }
  }
}

void test_parse_geojson_sans_geometry_from_file() {
  DEBUG_PRINTF("\n\nTEST GEOJSON PARSING SUBSET\n");
  File f = LittleFS.open("./big.geojson", "r");
  if (!f) {
    DEBUG_PRINTF("Failed to open file for reading\n");
    return;
  }

  FeatureCollectionSansGeometry fc;
  JSON::ParseResult pr = fc.fromJSON(&f);

  check(pr.error == 0, "parse error %s", errorToString(pr.error));
  check(fc.type == "FeatureCollection", "type == FeatureCollection, was %.*s",
        (int)fc.type.length(), fc.type.data());

  fc.toJSON(Serial, false);
}

void test_parse_with_callback_geojson_medium_from_stream(Stream *stream) {
  DEBUG_PRINTF("\n\nTEST GEOJSON MEDIUM PARSING WITH CALLBACK\n");
  StreamCursor cursor(stream);
  // arrayIndex depth should be 0 for the geometry array, 1 for the rings, 2 for the coordinates, 3 for the lon/lat
  // Ok got it. It is FeatureCollection → feature → geometry → coordinates → rings → coordinates → lon/lat
  //                          0              1           2           3           4           5           6
  // the depth level is increased by 1 for each array level but not for the object level
  FeatureMultipoint f;
  strncpy(f.type, "Feature", sizeof(f.type));
  strncpy(f.properties.name, "France", sizeof(f.properties.name));
  strncpy(f.geometry.type, "LineString", sizeof(f.geometry.type));

  size_t count = 0;
  JSON::ParseResult pr = JSON::parse(cursor, [&f, &count](const JSONKey &key, const JSONValue &value, JSON::SKIP &skip) {
    // print the lat/lon of the first coordinate of any ring of the first feature
    static float lon = 0.0f;

    if (!(key == "coordinates")) {
      return;
    }

    int16_t POLYGON_INDEX = key[-3];

    if (POLYGON_INDEX > 0) {
      skip = JSON::SKIP::STOP;
      return;
    }

    int16_t COORDINATES_INDEX = key[-1];

    if (COORDINATES_INDEX % 30 != 0) {
      return;
    }

    int16_t lon_lat_index = key.getArrayIndex();

    if (lon_lat_index == 0) {
      lon = value;
    } else if (lon_lat_index == 1) {
      //DEBUG_PRINTF("Ring#%d Coordinate: %f, %f\n", key.getArrayIndex(2), coordinate[0], coordinate[1]);
      f.geometry.coordinates.push_back({ lon, value });
      if (count++ > 500) skip = JSON::SKIP::STOP;
      //if (key[-1] >= 10) skip = JSON::SKIP::END;
    }
  });

  // Replace the last coordinate with the first one to close the LineString
  if (f.geometry.coordinates.size() > 0) {
    f.geometry.coordinates.back() = f.geometry.coordinates.front();
  }

  check(pr.error == JSON::NO_ERROR, "Parse ok");
/*
[FAIL] coordinates[0][0] == -65.613617f, was 2.521800
[FAIL] coordinates[0][1] == 43.420273f, was 51.087540
[FAIL] coordinates[1][0] == -65.613617f, was 2.762963
[FAIL] coordinates[1][1] == 43.420273f, was 50.739384
*/
  check(f.geometry.coordinates.size() > 0, "geometry.coordinates.size() > 0, was %u", f.geometry.coordinates.size());
  if (f.geometry.coordinates.size() > 0) {
    check(near(f.geometry.coordinates[0][0], 2.521800f), "coordinates[0][0] == 2.521800f, was %f", f.geometry.coordinates[0][0]);
    check(near(f.geometry.coordinates[0][1], 51.087540f), "coordinates[0][1] == 51.087540f, was %f", f.geometry.coordinates[0][1]);
    check(near(f.geometry.coordinates[1][0], 2.762963f), "coordinates[1][0] == 2.762963f, was %f", f.geometry.coordinates[1][0]);
    check(near(f.geometry.coordinates[1][1], 50.739384f), "coordinates[1][1] == 50.739384f, was %f", f.geometry.coordinates[1][1]);
  }

  f.toJSON(Serial);
  Serial.println("");
  Serial.printf("Parsing took %.02fs\n", (float)pr.elapsed / 1000000.0f);
}

void test_parse_with_callback_geojson_medium_from_file() {
  File f = LittleFS.open("./medium.geojson", "r");
  if (!f) {
    DEBUG_PRINTF("Failed to open file for reading\n");
    check(false, "Failed to open file for reading\n");
    return;
  }

  test_parse_with_callback_geojson_medium_from_stream(&f);
}

void test_parse_geojson_big_with_limited_geometry_from_stream(Stream *stream) {
  DEBUG_PRINTF("\n\nTEST GEOJSON PARSING SUBSET\n");

  FeatureCollectionLimited10Rings fc;
  JSON::ParseResult pr = fc.fromJSON(stream);

  check(pr.error == 0, "parse error == NO_ERROR, was %s %s %zu", errorToString(pr.error), errorToString(pr.parseError), pr.length);
  check(fc.type == "FeatureCollection", "type == FeatureCollection, was %.*s",
        (int)fc.type.length(), fc.type.data());

  check(near(fc.features[0].geometry.coordinates[0][0][0], -65.613617f), "coordinates[0][0][0] == -65.613617f, was %f", fc.features[0].geometry.coordinates[0][0][0]);
  check(near(fc.features[0].geometry.coordinates[0][0][1], 43.420273f), "coordinates[0][0][1] == 43.420273f, was %f", fc.features[0].geometry.coordinates[0][0][1]);
  check(near(fc.features[0].geometry.coordinates[4][0][0], -61.199997f), "coordinates[4][0][0] == -61.199997f, was %f", fc.features[0].geometry.coordinates[4][0][0]);
  check(near(fc.features[0].geometry.coordinates[4][0][1], 45.558327f), "coordinates[4][0][1] == 45.558327f, was %f", fc.features[0].geometry.coordinates[4][0][1]);
  check(near(fc.features[0].geometry.coordinates[9][0][0], -64.039719f), "coordinates[9][0][0] == -64.039719f, was %f", fc.features[0].geometry.coordinates[9][0][0]);
  check(near(fc.features[0].geometry.coordinates[9][0][1], 46.743324f), "coordinates[9][0][1] == 46.743324f, was %f", fc.features[0].geometry.coordinates[9][0][1]);

  fc.toJSON(Serial);
  Serial.println("");
}

void test_parse_geojson_big_with_limited_geometry_from_file() {
  File f = LittleFS.open("./big.geojson", "r");
  if (!f) {
    DEBUG_PRINTF("Failed to open file for reading\n");
    return;
  }
  test_parse_geojson_big_with_limited_geometry_from_stream(&f);
}

void test_parse_array_overflow() {
  DEBUG_PRINTF("\n\nTEST ARRAY OVERFLOW\n");
  IntegralArray s;
  const char *json = "{\"hex\":[1,2,3,4,5], \"unknown\":1}";
  JSON::ParseResult pr = s.fromJSON(json);

  check(pr.error == 0, "parse error %s", errorToString(pr.error));
  check(s.hex[0] == 1 && s.hex[1] == 2 && s.hex[2] == 3 && s.hex[3] == 4, "hex == [1 2 3 4], was [%d,%d,%d,%d]", s.hex[0], s.hex[1], s.hex[2], s.hex[3]);
}

void test_parse_array_array_overflow() {
  DEBUG_PRINTF("\n\nTEST ARRAY OVERFLOW\n");
  STDArrayArray s;
  const char *json = "{\"array\":[[1,2],[3,4],[5,6]], \"unknown\":1}";
  JSON::ParseResult pr = s.fromJSON(json);

  check(pr.error == 0, "parse error %s", errorToString(pr.error));
  check(s.array[0][0] == 1 && s.array[0][1] == 2, "ok");
  s.toJSON(Serial);
}

void test_parse_vector_array_array_overflow() {
  DEBUG_PRINTF("\n\nTEST ARRAY OVERFLOW\n");
  VectorArrayArray s;
  const char *json = "{\"vector\":[[[1,2],[3,4],[5,6]], [[1,2],[3,4],[5,6]]], \"unknown\":1}";
  JSON::ParseResult pr = s.fromJSON(json);
  check(pr.error == 0, "parse error %s", errorToString(pr.error));
  check(s.vector[0][0][0] == 1 && s.vector[1][1][0] == 3, "ok");
}

#ifndef ARDUINO

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

template<typename T>
void test_parse_geojson_big() {
  DEBUG_PRINTF("\n\nTEST GEOJSON PARSING BIG FILE\n");
  constexpr bool limited = std::is_same_v<T, FeatureCollectionLimited10Rings>;

  if constexpr (limited) {
    DEBUG_PRINTF("TESTING FEATURECOLLECTIONLIMITED\n");
  }
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  char *json = read_file("./big.geojson");
  T fc;

  // La ligne précédente ne marche pas !! remove_extent_t transforme int[N] en int mais pas Type<N> en Type !!
  // Il faut utiliser std::remove_all_extents_t pour les types composites.
  // J'ai essayé avec std::remove_all_extents_t mais ça ne marche pas non plus.
  // Je ne comprends pas pourquoi.
  // Question: comment transformer Type<N> en Type ?
  // Réponse: il faut utiliser std::remove_extent_t<T> pour les types simples et std::remove_all_extents_t<T> pour les types composites.
  uint64_t start = now();
  JSON::ParseResult pr = fc.fromJSON(json);
  [[maybe_unused]] uint64_t elapsed1 = now() - start;
  check(pr.error == 0, "parse error %u, parsed length=%zu", pr.error,
        pr.length);
  check(fc.type == "FeatureCollection", "type == FeatureCollection, was %.*s",
        (int)fc.type.length(), fc.type.data());
  check(fc.features.size() == 1, "1 feature, was %u", fc.features.size());
  if (fc.features.size() >= 1) {
    check(strcmp(fc.features[0].type, "Feature") == 0,
          "feature.type == Feature, was %s", fc.features[0].type);
    check(strcmp(fc.features[0].properties.name, "Canada") == 0,
          "properties.name == Canada, was %s", fc.features[0].properties.name);
    check(strcmp(fc.features[0].geometry.type, "Polygon") == 0,
          "geometry.type == Polygon, was %s", fc.features[0].geometry.type);
    check(fc.features[0].geometry.coordinates.size() == (limited ? 10 : 480), "%u rings, was %u", (limited ? 10 : 480), fc.features[0].geometry.coordinates.size());
    if (fc.features[0].geometry.coordinates.size() >= 2) {
      check(fc.features[0].geometry.coordinates[0].size() == (limited ? 1 : 14), "ring[0] has %u points, was %u", (limited ? 1 : 14), fc.features[0].geometry.coordinates[0].size());
      check(fc.features[0].geometry.coordinates[1].size() == (limited ? 1 : 33), "ring[1] has %u points, was %u", (limited ? 1 : 33), fc.features[0].geometry.coordinates[1].size());
      // Spot-check first coordinate of ring[0]: [-140.99778, 41.675105]
      check(
          near(fc.features[0].geometry.coordinates[0][0][0], -65.614f, 0.001f),
          "ring[0][0].lon ≈ -65.614, was %.3f",
          fc.features[0].geometry.coordinates[0][0][0]);
      if constexpr (!limited) {
        // check the last coordinate of the last ring of feature[0] from file big.geojson
        size_t coordinates_size = fc.features[0].geometry.coordinates.size();
        size_t last_ring_size = fc.features[0].geometry.coordinates[coordinates_size - 1].size();
        float lon = fc.features[0].geometry.coordinates[coordinates_size - 1][last_ring_size - 1][0];
        // -70.111937999999952
        check(near(lon, -70.111938f), "ring[%zu][%zu].lon ≈ -70.111938, was %.3f", coordinates_size - 1, last_ring_size - 1, lon);
      }
    }
  }

  // RAPIDJSON
  rapidjson::Document d;
  start = now();
  d.Parse(json);
  uint64_t elapsed2 = now() - start;
  free(json);

  float factor = (float)elapsed1 / (float)elapsed2;
  bool faster = elapsed1 < elapsed2;
  if (factor !=0 && faster) {
    factor = 1.0f / factor;
  }
  
  DEBUG_PRINTF("JSONParser Parsing time: %lu µs\n", elapsed1);
  DEBUG_PRINTF("RapidJSON Parsing time: %lu µs\n", elapsed2);
  DEBUG_PRINTF("JSONParser is %.2f times %s than RapidJSON\n", faster ? "faster" : "slower", factor);
}

void test_parse_geojson_big_with_limited_geometry() {
  test_parse_geojson_big<FeatureCollectionLimited10Rings>();
}

#endif
// ----------------------------------------------------------------
// Test 1 – fromJSON via char buffer
// ----------------------------------------------------------------

void test_parse_from_char_buffer() {
  DEBUG_PRINTF("\n--- Test: fromJSON via char buffer ---\n");

  const char *json = "{\"id\":42,\"name\":\"abc\",\"temperature\":23.5,"
                     "\"active\":true, \"num\":[1,2,3]}";

  Sensor s;
  JSON::ParseResult result = s.fromJSON(json);

  check(result.error == 0, "parse");
  check(s.id == 42, "id == 42");
  check(strcmp(s.name, "abc") == 0, "name == '%s'", s.name);
  check(s.active == true, "active == true");
  check(s.num[0] == 1 && s.num[1] == 2 && s.num[2] == 3, "num == [1,2,3]");
  check(near(s.temperature, 23.5f), "temperature ≈ 23.5");
}
// ----------------------------------------------------------------
// Test 1 – fromJSON via StreamString
// ----------------------------------------------------------------

void test_parse_from_stream() {
  DEBUG_PRINTF("\n--- Test: fromJSON via StreamString ---\n");

  const char *json = "{\"id\":42,\"temperature\":23.5,\"active\":true}";

  StreamString stream(json);

  Sensor s;
  s.id = 0;
  s.temperature = 0.0f;
  s.active = false;
  JSON::ParseResult result = s.fromJSON(stream);

  check(result.error == 0, "parse");
  check(s.id == 42, "id == 42");
  check(s.active == true, "active == true");
  check(near(s.temperature, 23.5f), "temperature ≈ 23.5");

  // s.toJSON(Serial);
}

// ----------------------------------------------------------------
// Test 2 – fromJSON partial update
// ----------------------------------------------------------------

void test_partial_parse() {
  DEBUG_PRINTF("\n--- Test: partial fromJSON via StreamString ---\n");

  Sensor s;
  s.id = 99;
  s.temperature = 10.0f;
  s.active = false;

  // Only update "active"
  const char *json = "{\"temperature\":20}";
  StreamString stream(json);
  JSON::ParseResult result = s.fromJSON(stream);

  check(result.error == 0, "parse");
  check(s.id == 99, "id unchanged (99)");
  check(s.temperature == 20, "temperature updated to 20");
}

void test_parse_json_subset() {
  DEBUG_PRINTF("\n--- Test: parse JSON subset ---\n");
  SensorMin s;
  const char *json = "{\"temp\":20, \"unrelated_1\":1, \"active\":false, "
                     "\"id\":42, \"unrelated_2\":1, \"unrelated_3\":1}";
  JSON::ParseResult result = s.fromJSON(json);
  check(result.error == 0, "parse");
  check(s.id == 42, "id == 42");
  check(s.active == false, "active == false");
  check(near(s.temp, 20U), "temp == 20");
  check(result.nParsed == 4, "nParsed == 4, was %zu", result.nParsed);
  check(result.nMatched == 3, "nMatched == 3, was %zu", result.nMatched);
  check(result.nConverted == 3, "nConverted == 3, was %zu", result.nConverted);
  check(result.nUpdated == 2, "nUpdated == 2, was %zu", result.nUpdated);
}

void test_parse_embedded_object_subset() {
  DEBUG_PRINTF("\n--- Test: parse embedded object subset ---\n");
  Parent p;  // Parent can decode the keys nom, age, child. Child decodes the
             // keys nom, prenom, age
  const char *json = "{\"nom\":\"Bob\",\"child\":{\"nom\":\"Legendre\", "
                     "\"prenom\":\"Alice\",\"age\":8, \"unknown\": "
                     "{\"key_1\":0, \"key_2\":[1,2,3]}},\"age\":40}";
  JSON::ParseResult result = p.fromJSON(json);
  check(result.error == 0, "parse %s", errorToString(result.error));
  check(result.nParsed == 3, "nParsed == 3, was %zu", result.nParsed);
  check(result.nMatched == 3, "nMatched == 3, was %zu", result.nMatched);
  check(p.age == 40, "age == 40, was %d", p.age);
}

void test_parse_json_subset2() {
  DEBUG_PRINTF("\n--- Test: parse JSON subset 2 ---\n");
  Parent p;  // Parent can decode the keys nom, age, child. Child decodes the
             // keys nom, prenom, age
  const char *json = "{\"nom\":\"Bob\",\"age\":40,\"child\":{\"nom\":\"Legendre\", "
                     "\"prenom\":\"Alice\",\"age\":8} , \"unknown\": {\"key_1\":0, \"key_2\":[1,2,3]}}";
  JSON::ParseResult result = p.fromJSON(json);
  check(result.error == 0, "parse %s", errorToString(result.error));
  check(result.nParsed == 3, "nParsed == 3, was %zu", result.nParsed);
  check(result.nMatched == 3, "nMatched == 3, was %zu", result.nMatched);
  check(p.age == 40, "age == 40, was %d", p.age);
}
// ----------------------------------------------------------------
// Test 4 – toJSON char buffer
// ----------------------------------------------------------------

void test_serialize_to_buffer() {
  DEBUG_PRINTF("\n--- Test: print to buffer ---\n");
  Sensor s;
  s.id = 7;
  s.temperature = 36;
  s.active = true;

  char buf[256] = {};
  size_t written = s.toJSON(buf);
  const char *expected = "{\"id\":7,\"active\":true,\"name\":\"\","
                         "\"temperature\":36,\"num\":[1,2,3]}";
  check(strcmp(expected, buf) == 0, "Wrote %zu bytes to char buffer: '%.*s'\n",
        written, (int)written, buf);
}

// ----------------------------------------------------------------
// Test 5 – toJSON via StreamString
// ----------------------------------------------------------------

void test_serialize_to_stream() {
  DEBUG_PRINTF("\n--- Test: toJSON via StreamString ---\n");

  Sensor s;
  s.id = 7;
  s.temperature = 36.55f;
  s.active = true;
  strncpy(s.name,
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789",
          sizeof(s.name));

  // Use a StreamString as the backing stream;
  StreamString stream;

  size_t written = s.toJSON(stream);
  check(written > 0, "toJSON wrote %zu bytes to StreamString: '%s'\n", written,
        stream.c_str());
}

// ----------------------------------------------------------------
// Test 6 – toJSON Serial
// ----------------------------------------------------------------

void test_print_to_serial() {
  DEBUG_PRINTF("\n--- Test: toJSON print to stdout ---\n");
  Sensor s;
  s.id = 7;
  s.temperature = 36.6f;
  s.active = true;

  s.toJSON(Serial);
  DEBUG_PRINTF("\n");
  check(true, "Wrote to Serial");
}
/*
void test_print_vector_to_serial() {
  DEBUG_PRINTF("\n--- Test: print vector property to stdout ---\n");

  VectorInt t;
  t.numbers.push_back(1);
  t.numbers.push_back(2);
  t.numbers.push_back(3);

  t.toJSON(Serial);
  DEBUG_PRINTF("\n");
}

void test_print_vector_of_arrays_to_serial() {
  DEBUG_PRINTF("\n--- Test: print std::array property to stdout ---\n");
  Ring r;
  r.coordinates.push_back({1, 2});
  r.coordinates.push_back({3, 4});
  r.coordinates.push_back({5, 6});

  r.toJSON(Serial);
  DEBUG_PRINTF("\n");
}

void test_print_vector_of_vectors_to_serial() {
  DEBUG_PRINTF("\n--- Test: print std::vector property to stdout ---\n");
  Geometry g;
  g.coordinates.push_back({{1, 2}, {3, 4}, {5, 6}, {7, 8}});
  g.coordinates.push_back({{9, 10}, {11, 12}, {13, 14}, {15, 16}});
  g.toJSON(Serial);
  DEBUG_PRINTF("\n");
}

void test_print_geojson_feature_to_serial() {
  DEBUG_PRINTF("\n--- Test: print geojson feature to stdout ---\n");
  Feature f;
  strncpy(f.type, "Feature", sizeof(f.type));
  strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
  strncpy(f.geometry.type, "Polygon", sizeof(f.geometry.type));
  f.geometry.coordinates.push_back({{1, 2}, {3, 4}, {5, 6}, {7, 8}});
  f.toJSON(Serial);
  DEBUG_PRINTF("\n");
}

void test_print_geojson_feature_sans_geometry_to_serial() {
  DEBUG_PRINTF("\n--- Test: print geojson feature to stdout ---\n");
  FeatureSansGeometry f;
  strncpy(f.type, "Feature", sizeof(f.type));
  strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
  f.toJSON(Serial);
  DEBUG_PRINTF("\n");
}

void test_print_geojson_to_serial() {
  DEBUG_PRINTF("\n--- Test: print geojson to stdout ---\n");
  FeatureCollection fc;
  fc.type = "FeatureCollection";
  Feature f;
  strncpy(f.type, "Feature", sizeof(f.type));
  strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
  strncpy(f.geometry.type, "Polygon", sizeof(f.geometry.type));
  f.geometry.coordinates.push_back({{1, 2}, {3, 4}, {5, 6}, {7, 8}});
  fc.features.push_back(f);
  fc.toJSON(Serial);
  DEBUG_PRINTF("\n");
}
*/
void test_print_geojson_to_buffer() {
  DEBUG_PRINTF("\n--- Test: print geojson to stdout ---\n");
  FeatureCollection fc;
  fc.type = "FeatureCollection";
  Feature f;
  strncpy(f.type, "Feature", sizeof(f.type));
  strncpy(f.properties.name, "feature_0", sizeof(f.properties.name));
  strncpy(f.geometry.type, "Polygon", sizeof(f.geometry.type));
  f.geometry.coordinates.push_back({ { 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } });
  fc.features.push_back(f);

  char buf[1024] = { 0 };
  fc.toJSON(buf);
  const char *expected = "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\",\"properties\":{\"name\":\"feature_0\"},\"geometry\":{\"type\":\"Polygon\",\"coordinates\":[[[1,2],[3,4],[5,6],[7,8]]]}}]}";
  check(strcmp(buf, expected) == 0, "toJSON output, should be \n%s\n, got \n%s", expected, buf);
}

void test_print_to_stream_string() {
  DEBUG_PRINTF("\n--- Test: toJSON StreamString ---\n");
  StreamString stream;
  CharArrayTest s;
  s.name = "name";
  s.numbers[0] = 12;
  s.numbers[1] = 10000;
  strncpy(s.names[0], "a", 32);
  strncpy(s.names[1], "b", 32);
  s.toJSON(stream);
  const char *expected =
    "{\"name\":\"name\",\"names\":[\"a\",\"b\",\"\"],\"numbers\":[12,10000]}";
  check(strcmp(stream.c_str(), expected) == 0,
        "toJSON output, should be %s, got %s", expected, stream.c_str());
}

void test_print_hex_to_stream_string() {
  DEBUG_PRINTF("\n--- Test: toJSON StreamString with HEX uint8_t array ---\n");
  StreamString stream;
  IntegralArray s;
  s.hex[0] = 0xAA;
  s.hex[1] = 0xBB;
  s.hex[2] = 0xCC;
  s.hex[3] = 0xDD;
  JSON::PRINT_BUFFER_AS_HEX = true;
  s.toJSON(stream);
  const char *expected = "{\"hex\":\"AABBCCDD\"}";
  check(strcmp(stream.c_str(), expected) == 0,
        "toJSON output, should be %s, got %s", expected, stream.c_str());
}

// ----------------------------------------------------------------
// Test 7 – roundtrip: parse then re-serialize
// ----------------------------------------------------------------

void test_roundtrip() {
  DEBUG_PRINTF("\n--- Test: roundtrip parse → serialize ---\n");

  Sensor original;
  original.id = 72;
  original.temperature = 19.8f;
  original.active = true;

  // Serialize original to a char buffer via PointerCursorWriter
  char buf[256] = { 0 };
  size_t len = original.toJSON(buf);
  Serial.printf("toJSON '%.*s'\n", (int)len, buf);

  StreamString stream(buf);
  Sensor copy;
  JSON::ParseResult result = copy.fromJSON(stream);

  check(result.error == 0, "parse");
  check(copy.id == 72, "id == 72");
  check(copy.active == true, "active == true");
  check(near(copy.temperature, 19.8f), "temperature ≈ 19.8, was %f",
        copy.temperature);
}

void test_print_to_file_stream() {

  const char *filename = "./sensor.json";

  DEBUG_PRINTF("\n--- Test: serialize to file ---\n");
  Sensor s1;
  s1.id = 8;
  s1.temperature = 36.6f;

  // delete file if it exists
  if (LittleFS.exists(filename)) {
    LittleFS.remove(filename);
  }
  // write sensor to file
  File file_w = LittleFS.open(filename, "w");
  if (!file_w) {
    DEBUG_PRINTF("Failed to open file for writing\n");
    return;
  }
  s1.toJSON(file_w, false);
  file_w.close();
  // read file back
  File file_r = LittleFS.open(filename, "r");
  if (!file_r) {
    DEBUG_PRINTF("Failed to open file for reading\n");
    check(false, "Failed to open file for reading\n");
    return;
  }

  DEBUG_PRINTF("FILE CONTENT ----\n");
  while (file_r.available() > 0) {
    DEBUG_PRINTF("%c", file_r.read());
  }
  DEBUG_PRINTF("\n-----------------\n");
  file_r.seek(0);

  Sensor s2;
  JSON::ParseResult result = s2.fromJSON(file_r);
  file_r.close();

  check(result.error == 0, "parse error %u", result.error);
  check(s2.id == 8, "id == 8, was %d", s2.id);
  check(near(s2.temperature, 36.6f), "temperature ≈ 36.6, was %f",
        s2.temperature);
  // check(std::memcmp(&s1, &s2, sizeof(Sensor)) == 0, "s1 == s2");

  LittleFS.remove(filename);
}

void test_parse_geojson_from_file() {
  DEBUG_PRINTF("\n--- Test: parse GeoJSON from file ---\n");

  // write geojson to file

  size_t size = generate_geojson(GEOJSON_TEST_FILE_PATH, 500);

  check(LittleFS.exists(GEOJSON_TEST_FILE_PATH),
        "Generated geojson file size=%zuB\n", size);

  // read file back
  File file = LittleFS.open(GEOJSON_TEST_FILE_PATH, "r");
  yield();

  if (!file) {
    DEBUG_PRINTF("Failed to open file for reading\n");
    check(false, "Failed to open file for reading\n");
    return;
  }

  FeatureCollection fc;
  JSON::ParseResult result = fc.fromJSON(&file);
  file.close();

  check(result.error == 0U, "parse error %u", result.error);

  // fc.toJSON(Serial, false);

  check(fc.type == "FeatureCollection", "type == FeatureCollection, was %.*s",
        (int)fc.type.length(), fc.type.data());

  if (fc.features.size() >= 1) {
    check(strcmp(fc.features[0].type, "Feature") == 0,
          "feature[0].type == Feature, was %s", fc.features[0].type);
    check(strcmp(fc.features[0].properties.name, "feature_0") == 0,
          "feature[0].properties.name == feature_0, was %s",
          fc.features[0].properties.name);
    check(strcmp(fc.features[0].geometry.type, "Polygon") == 0,
          "feature[0].geometry.type == Polygon, was %s",
          fc.features[0].geometry.type);
    check(fc.features[0].geometry.coordinates.size() == 1,
          "feature[0].geometry has 1 rings, was %u",
          fc.features[0].geometry.coordinates.size());

    if (fc.features[0].geometry.coordinates.size() >= 2) {
      check(fc.features[0].geometry.coordinates[0].size() == 5,
            "ring[0] has 5 points, was %u",
            fc.features[0].geometry.coordinates[0].size());
    }
  }

  // delete file
#if ARDUINO
  LittleFS.remove(GEOJSON_TEST_FILE_PATH);
#endif
}

// ----------------------------------------------------------------
// Test – BigStruct round-trip parse
// ----------------------------------------------------------------

void test_parse_big_struct() {
  DEBUG_PRINTF("\n--- Test: BigStruct parse ---\n");

  const char *json = "{"
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
  JSON::ParseResult r;

  uint64_t start = now();
  for (size_t i = 0; i < 10000UL; i++) {
    r = b.fromJSON(json);
    yield();
  }

  uint64_t elapsed = now() - start;
  check(true, "Parsing time: %.1f µs\n", (float)elapsed / 10000.0F);
  check(r.error == 0, "parse");
  check(b.f01 == true, "f01 == true");
  check(b.f02 == -12, "f02 == -12");
  check(b.f03 == 1000, "f03 == 1000");
  check(b.f04 == 123456, "f04 == 123456");
  check(b.f05 == 4000000000LL, "f05 == 4000000000");
  check(b.f06 == 200, "f06 == 200");
  check(b.f07 == 50000, "f07 == 50000");
  check(b.f08 == 3000000000U, "f08 == 3000000000");
  check(near(b.f09, 1.5f), "f09 ≈ 1.5");
  check(near((float)b.f10, 2.71828f), "f10 ≈ 2.71828");
  check(strcmp(b.f11, "hello") == 0, "f11 == 'hello'");
  check(strcmp(b.f12, "world_string") == 0, "f12 == 'world_string'");
  check(b.f13 == std::string_view("view_one"), "f13 == 'view_one'");
  check(b.f14 == std::string_view("view_two"), "f14 == 'view_two'");
  check(b.f15 == false, "f15 == false");
  check(b.f16 == -999, "f16 == -999");
  check(near(b.f17, 3.14f), "f17 ≈ 3.14");
  check(near((float)b.f18, 1.41421f), "f18 ≈ 1.41421");
  check(b.f19[0] == 1 && b.f19[1] == 2 && b.f19[2] == 3 && b.f19[3] == 4,
        "f19 == [1,2,3,4]");
  check(b.f20[0] == 100 && b.f20[1] == 200 && b.f20[2] == 300 && b.f20[3] == 400,
        "f20 == [100,200,300,400]");
  check(b.f21[0] == 1000 && b.f21[1] == 2000 && b.f21[2] == 3000 && b.f21[3] == 4000,
        "f21 == [1000,2000,3000,4000]");
  check(near(b.f22[0], 1.1f) && near(b.f22[1], 2.2f) && near(b.f22[2], 3.3f) && near(b.f22[3], 4.4f),
        "f22 ≈ [1.1,2.2,3.3,4.4]");
  check(b.f23[0] == -1 && b.f23[1] == -2 && b.f23[2] == -3 && b.f23[3] == -4,
        "f23 == [-1,-2,-3,-4]");
  check(b.f24 == 42, "f24 == 42");
  check(b.f25 == -500, "f25 == -500");
  check(b.f26 == 255, "f26 == 255");
  check(b.f27 == 65000, "f27 == 65000");
  check(b.f28 == true, "f28 == true");
  check(near((float)b.f29, 0.12345f), "f29 ≈ 0.12345");
  check(strcmp(b.f30, "short") == 0, "f30 == 'short'");
  check(b.f31 == std::string_view("last_view"), "f31 == 'last_view'");
  check(b.f32 == 77, "f32 == 77");
}

void test_parse_escape_sequence_from_buffer() {
  DEBUG_PRINTF("\n--- Test: parse escape sequence ---\n");
  const char *json =
    "{\"prenom\":\"Jean dit \\\"Jeannot\\\" ou Jo\", \"nom\":\"Legendre\"}";
  Child p;
  JSON::ParseResult r = p.fromJSON(json);
  check(r.error == 0, "parse");
  check(p.prenom == std::string_view("Jean dit \"Jeannot\" ou Jo"),
        "prenom == 'Jean dit \"Jeannot\" ou Jo' , was '%.*s'",
        (int)p.prenom.length(), p.prenom.data());
  check(p.nom == std::string_view("Legendre"), "nom == 'Legendre' , was '%.*s'",
        (int)p.nom.length(), p.nom.data());
}

void test_parse_escape_sequence_from_stream() {
  DEBUG_PRINTF("\n--- Test: parse escape sequence with stream ---\n");
  const char *json =
    "{\"prenom\":\"Jean dit \\\"Jeannot\\\" ou Jo\", \"nom\":\"Legendre\"}";
  StreamString stream(json);
  Child p;
  JSON::ParseResult r = p.fromJSON(stream);
  check(r.error == 0, "parse");
  check(p.prenom == std::string_view("Jean dit \"Jeannot\" ou Jo"),
        "prenom == 'Jean dit \"Jeannot\" ou Jo' , was '%.*s'",
        (int)p.prenom.length(), p.prenom.data());
  check(p.nom == std::string_view("Legendre"), "nom == 'Legendre' , was '%.*s'",
        (int)p.nom.length(), p.nom.data());
}

void test_parse_strings_from_buffer() {
  DEBUG_PRINTF("\n--- Test: parse strings ---\n");
  Child enfant;
  const char *json = "{\"nom\":\"Legendre\", \"prenom\":\"Jean\", \"age\":42}";
  JSON::ParseResult r = enfant.fromJSON(json);
  check(r.error == 0, "parse");
  check(enfant.nom == std::string_view("Legendre"),
        "nom == 'Legendre' , was '%.*s'", (int)enfant.nom.length(),
        enfant.nom.data());
  check(enfant.prenom == std::string_view("Jean"),
        "prenom == 'Jean' , was '%.*s'", (int)enfant.prenom.length(),
        enfant.prenom.data());
}

void test_parse_strings_from_stream() {
  DEBUG_PRINTF("\n--- Test: parse strings ---\n");
  Child enfant;
  const char *json = "{\"nom\":\"Legendre\", \"prenom\":\"Jean\", \"age\":42}";
  StreamString stream(json);
  JSON::ParseResult r = enfant.fromJSON(stream);
  check(r.error == 0, "parse");
  check(enfant.nom == std::string_view("Legendre"),
        "nom == 'Legendre' , was '%.*s'", (int)enfant.nom.length(),
        enfant.nom.data());
  check(enfant.prenom == std::string_view("Jean"),
        "prenom == 'Jean' , was '%.*s'", (int)enfant.prenom.length(),
        enfant.prenom.data());
}

void test_parse_geojson_from_stream(Stream *stream) {
  FeatureCollection fc;
  JSON::ParseResult r = fc.fromJSON(stream);
  check(r.error == 0, "parse error = %s", JSON::errorToString(r.error));
}

void run_parsing_tests() {
  // with callback
  test_parse_callback();
  test_parse_array_callback();

  test_parsing();
  test_parse_indexed_keys();
  test_parse_top_level_array();
  test_parse_embedded_object();
  test_parse_embedded_object_from_stream();
  test_parse_from_char_buffer();
  test_parse_from_stream();
  test_partial_parse();
  test_parse_geojson_small();
  test_parse_geojson_from_file();
  test_parse_big_struct();
  test_parse_escape_sequence_from_buffer();
  test_parse_escape_sequence_from_stream();
  test_parse_strings_from_buffer();
  test_parse_strings_from_stream();
  test_parse_json_subset();
  test_parse_json_subset2();
  test_parse_embedded_object_subset();
  test_parse_geojson_big_with_limited_geometry_from_file();
#ifndef ARDUINO
  test_parse_geojson_big<FeatureCollection>();
  test_parse_geojson_big_with_limited_geometry();
  test_parse_with_callback_geojson_medium_from_file();
#endif
}

void run_printing_tests() {
  // to char buffer
  test_serialize_to_buffer();
  test_print_geojson_to_buffer();
  // to serial
  test_print_to_serial();
  // to stream
  test_print_to_stream_string();
  test_print_hex_to_stream_string();
  test_serialize_to_stream();
  test_print_to_file_stream();
}

bool run_tests() {
  [[maybe_unused]] time_t now = time(nullptr);
  DEBUG_PRINTF("TIME:%s COMPILER:%s", ctime(&now), __VERSION__);
  DEBUG_PRINTF(
    "------------------------------------------------------------\n");

  run_parsing_tests();
  run_printing_tests();
  test_roundtrip();

  DEBUG_PRINTF(
    "\n============================================================\n");
  DEBUG_PRINTF("Results: %d passed, %d failed\n", passed, failed);
  DEBUG_PRINTF(
    "============================================================\n");

  return failed == 0;
}
