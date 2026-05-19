#include "src/constants.h"

using namespace std;
using namespace JSON;

// ----------------------------------------------------------------
// Test infrastructure
// ----------------------------------------------------------------

static int passed = 0;
static int failed = 0;

template <typename... Args>
static void check(bool condition, const char *format, Args &&...args) {
  if (condition) {
    DEBUG_PRINTF("%*c[PASS] ", 2, ' ');
    ++passed;
  } else {
    DEBUG_PRINTF("%*c[FAIL] ", 2, ' ');
    ++failed;
  }

  if constexpr (sizeof...(Args) > 0) {
    char _buf[512];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
    snprintf(_buf, sizeof(_buf), format, std::forward<Args>(args)...);
#pragma clang diagnostic pop
    DEBUG_PRINTF("%s\n", _buf);
  } else {
    DEBUG_PRINTF("%s\n", format);
  }
}

static bool near(float a, float b, float tol = 0.01f) {
  float d = a - b;
  return d > -tol && d < tol;
}

// ----------------------------------------------------------------
// Structs
// ----------------------------------------------------------------

struct Sensor : public JSONObject {
  int id = 0;
  float temperature = 1.0f;
  bool active = false;
  char name[64] = {0};
  uint8_t num[3] = {1, 2, 3};
  JSON_SERIALIZE_IMPL(id, active, name, temperature, num);
};

struct Config : public JSONObject {
  int version = 0;
  float interval = 0.0f;
  JSON_SERIALIZE_IMPL(version, interval);
};

struct CharArrayTest : JSONObject {
  std::string_view name = "";
  char names[3][32] = {{'\0'}};
  uint32_t numbers[2] = {0};

  JSON_ENCODER_IMPL(name, names, numbers);
};

struct IntegralArrayTest : JSONObject {
  uint8_t hex[4] = {0};

  JSON_ENCODER_IMPL(hex);
};

struct Personne : public JSONObject {
public:
  std::string_view nom = "";
  uint8_t age = 0U;
  float taille = 0.0F;
  std::string_view ville = "";
  char *ptr;
  bool flag = false;

  uint8_t buffer[4] = {0};
  char liste[3][32] = {{'\0'}};
  float listef[5] = {0};
  Personne *enfant;
  std::vector<Personne> enfants;
  float coordinates[4][2];
  vector<std::array<float, 2>> coordinates2;

  Personne() : JSONObject() {}
  Personne(std::string_view nom, int age, float taille, std::string_view ville,
           char *ptr, bool flag, Personne *enfant)
      : JSONObject(), nom(nom), age(age), taille(taille), ville(ville),
        ptr(ptr), flag(flag), enfant(enfant) {}

  JSON_SERIALIZE_IMPL(nom, age, taille, ville, ptr, flag, buffer, liste, listef,
                      enfant, enfants, coordinates);
};

struct Properties : public JSONObject {
  std::string_view name = "";
  JSON_SERIALIZE_IMPL(name);
};

struct Geometry : public JSONObject {
  std::string_view type = "";
  using coordinate = std::array<float, 2>;
  using shape = std::vector<coordinate>;
  std::vector<shape> coordinates;
  JSON_SERIALIZE_IMPL(type, coordinates);
};

struct Feature : public JSONObject {
  std::string_view type = "";
  Properties properties;
  Geometry geometry;
  JSON_SERIALIZE_IMPL(type, properties, geometry);
};

struct FeatureCollection : public JSONObject {
  std::string_view type = "";
  std::vector<Feature> features;
  JSON_SERIALIZE_IMPL(type, features);
};

// ----------------------------------------------------------------
// Timing helper
// ----------------------------------------------------------------

uint64_t time() {
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
             now.time_since_epoch())
      .count();
}

// ----------------------------------------------------------------
// test_callback
// ----------------------------------------------------------------

void test_callback() {
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
      json, [&p, &liste_idx](JSONKey key, JSONValue value, bool &stop) {
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
  check(p.buffer[0] == 0xAA && p.buffer[1] == 0xBB && p.buffer[2] == 0xCC &&
            p.buffer[3] == 0xDD,
        "buffer == {AA BB CC DD}");
  check(strcmp(p.liste[0], "a") == 0, "liste[0] == 'a'");
  check(strcmp(p.liste[1], "b") == 0, "liste[1] == 'b'");
  check(strcmp(p.liste[2], "c") == 0, "liste[2] == 'c'");
}

// ----------------------------------------------------------------
// testArrayCallback
// ----------------------------------------------------------------

void testArrayCallback() {
  DEBUG_PRINTF("\nTEST ARRAY CALLBACK\n");
  DEBUG_PRINTF(
      "------------------------------------------------------------\n");

  size_t p_length = 3;
  Personne personnes[p_length];

  // age of personnes[0] is Infinity — not a valid number, stays 0
  const char *json = "[{\"nom\":\"Bob\",\"age\":Infinity},{\"nom\":\"Alice\","
                     "\"age\":30},{\"nom\":\"Roger\",\"age\":64}]";

  JSON::ParseResult pr = JSON::parse(
      json, [&personnes, p_length](JSONKey key, JSONValue value, bool &stop) {
        int arrayIndex = key.getArrayIndex();
        if (arrayIndex >= (int)p_length || arrayIndex < 0)
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
          stop = true;
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

// ----------------------------------------------------------------
// test_parsing
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

  check(result.error == 0, "parse");
  check(p.ville == std::string_view("Lyon"), "ville == Lyon");
  check(p.age == 45, "age == 45");
  check(near(p.taille, 1.82f), "taille ≈ 1.82");
  check(p.flag == false, "flag == false");
  check(p.ptr == nullptr, "ptr == nullptr (null)");
  check(p.buffer[0] == 170 && p.buffer[1] == 171 && p.buffer[2] == 172 &&
            p.buffer[3] == 173,
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
// testArrayParsing
// ----------------------------------------------------------------

void testArrayParsing() {
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
// testIndexedParsing
// ----------------------------------------------------------------

void testIndexedParsing() {
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
  check(mask == 3, "mask == 3 (bits 0 and 1 set)");
}

// ----------------------------------------------------------------
// GeoJSON helpers
// ----------------------------------------------------------------

// void testGeoJSONParsing(const char *json, bool print_result) {
//   if (print_result) {
//     fc.toJSON(Serial, false);
//     DEBUG_PRINTF("\n");
//     DEBUG_PRINTF("Parsed %zu features\n", fc.features.size());

//     if (fc.features.size() > 0) {
//       size_t shapes_length = fc.features[0].geometry.coordinates.size();
//       DEBUG_PRINTF("Shapes length =%zu\n", shapes_length);
//       for (size_t i = 0; i < shapes_length; i++) {
//         DEBUG_PRINTF("Shape %zu coordinate points length =%zu\n", i,
//                      fc.features[0].geometry.coordinates[i].size());
//       }
//     }
//   }
// }

void testGeoJSONParsingSmall() {
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
  JSON::ParseResult pr = fc.fromJSON(json);

  check(pr.error == 0, "parse");
  check(fc.type == std::string_view("FeatureCollection"),
        "type == FeatureCollection");
  check(fc.features.size() == 1, "1 feature");
  if (fc.features.size() >= 1) {
    check(fc.features[0].type == std::string_view("Feature"),
          "feature.type == Feature");
    check(fc.features[0].properties.name == std::string_view("Canada"),
          "properties.name == Canada");
    check(fc.features[0].geometry.type == std::string_view("Polygon"),
          "geometry.type == Polygon");
    check(fc.features[0].geometry.coordinates.size() == 3, "3 rings");
    if (fc.features[0].geometry.coordinates.size() >= 2) {
      check(fc.features[0].geometry.coordinates[0].size() == 5,
            "ring[0] has 5 points");
      check(fc.features[0].geometry.coordinates[1].size() == 5,
            "ring[1] has 5 points");
      // Spot-check first coordinate of ring[0]: [-140.99778, 41.675105]
      check(near(fc.features[0].geometry.coordinates[0][0][0], -140.99778f,
                 0.001f),
            "ring[0][0].lon ≈ -140.998");
      check(near(fc.features[0].geometry.coordinates[0][0][1], 41.675105f,
                 0.001f),
            "ring[0][0].lat ≈ 41.675");
    }
  }
}

void testGeoJSONParsingBig() {
  DEBUG_PRINTF("\n\nTEST GEOJSON PARSING BIG FILE\n");
  DEBUG_PRINTF(
      "------------------------------------------------------------\n");

  FILE *file = fopen("tests/canada.json", "r");
  if (!file) {
    DEBUG_PRINTF("ERROR: Could not open tests/canada.json\n");
    return;
  }
  fseek(file, 0, SEEK_END);
  long long fsize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *json = (char *)malloc(fsize + 1);
  size_t len = fread(json, 1, fsize, file);
  if (len != fsize) return;
  
  fclose(file);
  json[fsize] = 0;
    FeatureCollection fc;
    JSON::ParseResult pr = fc.fromJSON(json);

  #if defined(APPLE) || defined(__linux__)
    uint64_t start = now();
    rapidjson::Document d;
    d.Parse(json);
    [[maybe_unused]] uint64_t elapsed = now() - start;
    DEBUG_PRINTF("RapidJSON Parsing time: %lu µs\n", elapsed);
  #endif

    DEBUG_PRINTF("JSONParser Parsing time: %lu µs\n", pr.elapsed);

    check(pr.error == 0, "parse");
  free(json);
}
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
  // s.toJSON(Serial);
}

// ----------------------------------------------------------------
// Test 3 – fromJSON with StreamString
// ----------------------------------------------------------------

void test_parse_via_stream_template() {
  DEBUG_PRINTF("\n--- Test: fromJSON(Stream&) template helper ---\n");

  const char *json = "{\"version\":3,\"interval\":0.5}";
  StreamString stream(json);

  Config c;
  JSON::ParseResult result = c.fromJSON(stream);

  check(result.error == 0, "parse");
  check(c.version == 3, "version == 3");
  check(near(c.interval, 0.5f), "interval ≈ 0.5");
  // c.toJSON(Serial);
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

void test_print_to_stdout() {
  DEBUG_PRINTF("\n--- Test: toJSON print ---\n");
  char *ptr = (char *)"ptr";
  Personne p("Bob", 40, 1.80f, "Paris", ptr, false, nullptr);
  p.toJSON(Serial, false);
  DEBUG_PRINTF("\n");

  check(true, "You should see on the previous line the JSON representation of "
              "the Personne object");
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
  check(true, "Wrote to Serial");
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
        "toJSON output should be %s, got %s", expected, stream.c_str());
}

// ----------------------------------------------------------------
// Test 5 – toJSON via StreamString with HEX unsigned char array
// ----------------------------------------------------------------

void test_print_hex_to_stream_string() {
  DEBUG_PRINTF("\n--- Test: toJSON StreamString with HEX uint8_t array ---\n");
  StreamString stream;
  IntegralArrayTest s;
  s.hex[0] = 0xAA;
  s.hex[1] = 0xBB;
  s.hex[2] = 0xCC;
  s.hex[3] = 0xDD;

  JSON::PRINT_BUFFER_AS_HEX = true;
  s.toJSON(stream);
  JSON::PRINT_BUFFER_AS_HEX = false;

  const char *expected = "{\"hex\":\"AABBCCDD\"}";
  check(strcmp(stream.c_str(), expected) == 0,
        "toJSON output should be %s, got %s", expected, stream.c_str());
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
  char buf[256] = {0};
  size_t len = original.toJSON(buf);
  Serial.printf("toJSON '%.*s'\n", (int)len, buf);

  StreamString stream(buf);
  Sensor copy;
  JSON::ParseResult result = copy.fromJSON(stream);

  check(result.error == 0, "parse");
  check(copy.id == 72, "id == 72");
  check(copy.active == true, "active == true");
  check(near(copy.temperature, 19.8f), "temperature ≈ 19.8 and was %f",
        copy.temperature);
}

void run_parsing_tests() {
  test_callback();
  testArrayCallback();

  test_parsing();
  testIndexedParsing();
  testArrayParsing();
  test_parse_from_char_buffer();
  test_parse_from_stream();
  test_partial_parse();
  test_parse_via_stream_template();

  testGeoJSONParsingSmall();
}

void run_printing_tests() {
  // to char buffer
  test_serialize_to_buffer();
  // to serial
  test_print_to_serial();
  test_print_to_stdout();

  // to stream
  test_print_to_stream_string();
  test_print_hex_to_stream_string();
  test_serialize_to_stream();
}

void run_tests() {
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
}
